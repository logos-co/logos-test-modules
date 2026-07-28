#!/usr/bin/env python3
"""The `full_api` contract exists in five hand-maintained copies. Check them.

  test-fullapi-module-cpp/src/test_fullapi_cpp_impl.h        C++ provider (the
                                                             contract is DERIVED
                                                             from this header)
  test-fullapi-module-rust/rust-lib/test_fullapi_rust.lidl    Rust provider
  test-fullapi-proxy-module-rust/full_api.lidl                the shared interface
  test-fullapi-proxy-module-cpp/interfaces/full_api.h         C++ consumer's copy
  test-fullapi-proxy-module-rust/rust-lib/…_proxy_rust.lidl   Rust proxy re-export

Nothing enforced that they agree. A method added to one and forgotten in another
produces no error anywhere: the C++ provider's contract is derived from its own
header, the proxies compile against their own copies, and the matrix only ever
talks to the providers. The drift surfaces much later as "the interface does not
have that method".

This compares the method and event SIGNATURES — name, parameter types in order,
return type — across all five, in LIDL spelling. C++ declarations are mapped
through a small closed table; an unrecognised C++ spelling is a FAILURE, not a
skip, because silently ignoring a type is how a checker like this becomes
decorative.

Exit non-zero on any disagreement.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

COPIES = {
    "provider-cpp":   ROOT / "test-fullapi-module-cpp/src/test_fullapi_cpp_impl.h",
    "provider-rust":  ROOT / "test-fullapi-module-rust/rust-lib/test_fullapi_rust.lidl",
    "interface-lidl": ROOT / "test-fullapi-proxy-module-rust/full_api.lidl",
    "interface-h":    ROOT / "test-fullapi-proxy-module-cpp/interfaces/full_api.h",
    "proxy-rust":     ROOT / "test-fullapi-proxy-module-rust/rust-lib/test_fullapi_proxy_rust.lidl",
}

# The closed set of C++ spellings these contracts use. Anything else is a
# failure — see the module docstring.
CPP_TO_LIDL = {
    "std::string": "tstr",
    "std::vector<uint8_t>": "bstr",
    "int64_t": "int",
    "uint64_t": "uint",
    "double": "float64",
    "bool": "bool",
    "nlohmann::json": "any",
    "LogosMap": "{tstr:any}",
    "LogosList": "[any]",
    "std::vector<std::string>": "[tstr]",
    "std::vector<int64_t>": "[int]",
    "std::vector<uint64_t>": "[uint]",
    "std::vector<double>": "[float64]",
    "std::vector<bool>": "[bool]",
    "StdLogosResult": "result",
    "void": "void",
}

_LIDL_METHOD = re.compile(r"^\s*method\s+(\w+)\s*\((.*?)\)\s*->\s*(.+?)\s*$")
_LIDL_EVENT = re.compile(r"^\s*event\s+(\w+)\s*\((.*?)\)\s*$")
_CPP_DECL = re.compile(r"^\s*([\w:<>,\s]+?)\s+(\w+)\s*\((.*?)\)\s*;\s*$")


def norm(t: str) -> str:
    return re.sub(r"\s+", "", t)


def split_params(text: str) -> list[str]:
    out, depth, cur = [], 0, ""
    for ch in text:
        if ch in "[{<":
            depth += 1
        elif ch in "]}>":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [p.strip() for p in out if p.strip()]


def parse_lidl(path: Path):
    methods, events = {}, {}
    for line in path.read_text().splitlines():
        m = _LIDL_METHOD.match(line)
        if m:
            name, params, ret = m.groups()
            methods[name] = ([norm(p.split(":", 1)[1]) for p in split_params(params) if ":" in p],
                             norm(ret))
            continue
        m = _LIDL_EVENT.match(line)
        if m:
            name, params = m.groups()
            events[name] = [norm(p.split(":", 1)[1]) for p in split_params(params) if ":" in p]
    return methods, events


def cpp_type(decl: str, unknown: list) -> str:
    """A C++ declaration fragment -> its LIDL spelling."""
    t = decl.strip()
    t = re.sub(r"^const\s+", "", t)
    t = re.sub(r"\s*&$", "", t).strip()
    t = norm(t)
    for cpp, lidl in CPP_TO_LIDL.items():
        if norm(cpp) == t:
            return lidl
    unknown.append(decl.strip())
    return f"<unknown:{decl.strip()}>"


def parse_cpp(path: Path, unknown: list):
    """Methods before `logos_events:`; events after it."""
    methods, events = {}, {}
    in_events = False
    for line in path.read_text().splitlines():
        stripped = line.strip()
        if stripped.startswith("logos_events:"):
            in_events = True
            continue
        if stripped.startswith("//") or not stripped.endswith(";"):
            continue
        m = _CPP_DECL.match(line)
        if not m:
            continue
        ret, name, params = m.groups()
        if name in ("if", "while", "for", "return"):
            continue
        ptypes = []
        for p in split_params(params):
            # strip the parameter NAME: everything up to the last identifier
            frag = re.sub(r"\s+\w+$", "", p.strip())
            ptypes.append(cpp_type(frag, unknown))
        if in_events:
            events[name] = ptypes
        else:
            methods[name] = (ptypes, cpp_type(ret, unknown))
    return methods, events


def main() -> int:
    unknown: list[str] = []
    parsed = {}
    for label, path in COPIES.items():
        if not path.is_file():
            print(f"MISSING copy: {label} -> {path}")
            return 1
        parsed[label] = (parse_cpp(path, unknown) if path.suffix == ".h"
                         else parse_lidl(path))

    if unknown:
        print("C++ spellings not in the translation table — add them to "
              "CPP_TO_LIDL rather than letting the check skip a type:")
        for u in sorted(set(unknown)):
            print(f"  {u}")
        return 1

    # `whoAmI` is intentionally per-provider, and the proxies add their own
    # control surface (useProvider / lastEvent / probe*), so compare the
    # INTERSECTION of names — plus flag anything the shared interface declares
    # that a copy is missing, which is the drift that actually bites.
    reference = "interface-lidl"
    ref_methods, ref_events = parsed[reference]

    problems = []
    for label, (methods, events) in parsed.items():
        if label == reference:
            continue
        for name, sig in ref_methods.items():
            if name not in methods:
                problems.append(f"{label}: missing method `{name}` "
                                f"(the shared interface declares it)")
            elif methods[name] != sig:
                problems.append(
                    f"{label}: method `{name}` is {methods[name]}, "
                    f"the shared interface says {sig}")
        for name, params in ref_events.items():
            if name not in events:
                problems.append(f"{label}: missing event `{name}` "
                                f"(the shared interface declares it)")
            elif events[name] != params:
                problems.append(
                    f"{label}: event `{name}` takes {events[name]}, "
                    f"the shared interface says {params}")

    if problems:
        print(f"full_api contract copies disagree ({len(problems)}):")
        for p in problems:
            print(f"  {p}")
        return 1

    print(f"full_api contract: {len(COPIES)} copies agree "
          f"({len(ref_methods)} methods, {len(ref_events)} events)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
