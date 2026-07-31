#!/usr/bin/env python3
"""The `full_api` contract exists in eight hand-maintained copies. Check them.

  test-fullapi-module-cpp/src/test_fullapi_cpp_impl.h        C++ provider (the
                                                             contract is DERIVED
                                                             from this header)
  test-fullapi-module-rust/rust-lib/test_fullapi_rust.lidl    Rust provider
  test-fullapi-proxy-module-rust/full_api.lidl                the shared interface
  test-fullapi-proxy-module-cpp/interfaces/full_api.h         C++ consumer's copy
  test-fullapi-proxy-module-rust/rust-lib/…_proxy_rust.lidl   Rust proxy re-export
  test-fullapi-ui-module/interfaces/full_api.h                UI plugin's copy
  test-fullapi-qtproxy-module/interfaces/full_api.lidl        Qt consumer's copy
  test-fullapi-qtproxy-module/src/…_qtproxy_impl.h            Qt consumer's
                                                              RE-EXPOSED surface

Nothing enforced that they agree. A method added to one and forgotten in another
produces no error anywhere: the C++ provider's contract is derived from its own
header, the proxies compile against their own copies, and the matrix only ever
talks to the providers. The drift surfaces much later as "the interface does not
have that method".

This compares the method and event SIGNATURES — name, parameter types in order,
return type — across all copies, in LIDL spelling. C++ declarations are mapped
through small closed tables; an unrecognised C++ spelling is a FAILURE, not a
skip, because silently ignoring a type is how a checker like this becomes
decorative.

Three parse kinds, because the copies are not written in one language:

  lidl     a .lidl contract — exact comparison.
  cpp      a std-typed C++ header (`std::string`, `LogosMap`, …) whose events
           live under a `logos_events:` label — exact comparison.
  cpp-qt   a QT-typed provider header: only `LOGOS_METHOD`-marked declarations
           count, and the comparison is by COMPATIBILITY rather than equality
           because the Qt spellings are not 1:1 with LIDL — `QVariantList` is
           the single type behind `[any]`, `[int]`, `[uint]`, `[float64]` and
           `[bool]`. That collapse is a real property of the Qt api style, not
           a shortcut here; see the note next to QT_TO_LIDL.
           Its events are NOT compared: a Qt provider emits through
           `emitEvent(name, QVariantList)` and has no declared event block, so
           there is nothing to read. What keeps them honest instead is the
           compiler — the module subscribes through the generated typed
           `onXxxEvent` accessors, so an event that leaves the contract stops
           the build.

Exit non-zero on any disagreement.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# label -> (path, kind)
COPIES = {
    "provider-cpp":   (ROOT / "test-fullapi-module-cpp/src/test_fullapi_cpp_impl.h", "cpp"),
    "provider-rust":  (ROOT / "test-fullapi-module-rust/rust-lib/test_fullapi_rust.lidl", "lidl"),
    "interface-lidl": (ROOT / "test-fullapi-proxy-module-rust/full_api.lidl", "lidl"),
    "interface-h":    (ROOT / "test-fullapi-proxy-module-cpp/interfaces/full_api.h", "cpp"),
    "proxy-rust":     (ROOT / "test-fullapi-proxy-module-rust/rust-lib/test_fullapi_proxy_rust.lidl", "lidl"),
    # The UI plugin consumes the same contract and was the one copy nobody
    # checked — it had silently lost echoTriple / fireTripleEvent / tripleEvent
    # when multi-arg arity was added, which is exactly the drift this script
    # exists to catch.
    "ui-h":           (ROOT / "test-fullapi-ui-module/interfaces/full_api.h", "cpp"),
    # The Qt-typed consumer (the matrix's third consumer surface). Two copies:
    # the interface it binds, and the surface it re-exposes. The second is the
    # one that can drift silently — the generated `FullApi` wrapper always has
    # all 33 methods, so forgetting to forward one is not a compile error, it is
    # a matrix cell that quietly stops being covered.
    "qtproxy-lidl":   (ROOT / "test-fullapi-qtproxy-module/interfaces/full_api.lidl", "lidl"),
    "qtproxy-h":      (ROOT / "test-fullapi-qtproxy-module/src/test_fullapi_qtproxy_impl.h", "cpp-qt"),
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

# The Qt spellings, mapping to the SET of LIDL types each can stand for. Five
# entries are not 1:1, and that is the finding rather than a convenience here:
# the Qt api style has one list type, `QVariantList`, behind `[any]`, `[int]`,
# `[uint]`, `[float64]` and `[bool]` — a Qt-typed consumer cannot express which
# of the five it is (the lp/std style keeps them apart: std::vector<int64_t> vs
# LogosList). So this copy is checked for NAME and ARITY exactly, and for types
# up to that collapse; a `[uint]` written as `QVariantList` passes, a `[uint]`
# written as `QString` does not.
QT_TO_LIDL = {
    "QString": {"tstr"},
    "QByteArray": {"bstr"},
    "qlonglong": {"int"},
    "qulonglong": {"uint"},
    "double": {"float64"},
    "bool": {"bool"},
    "QVariant": {"any"},
    "QVariantMap": {"{tstr:any}"},
    "QStringList": {"[tstr]"},
    "QVariantList": {"[any]", "[int]", "[uint]", "[float64]", "[bool]"},
    "LogosResult": {"result"},
    "void": {"void"},
}

_LIDL_METHOD = re.compile(r"^\s*method\s+(\w+)\s*\((.*?)\)\s*->\s*(.+?)\s*$")
_LIDL_EVENT = re.compile(r"^\s*event\s+(\w+)\s*\((.*?)\)\s*$")
_CPP_DECL = re.compile(r"^\s*([\w:<>,\s]+?)\s+(\w+)\s*\((.*?)\)\s*;\s*$")
_QT_DECL = re.compile(r"^\s*LOGOS_METHOD\s+([\w:<>,\s*&]+?)\s+(\w+)\s*\((.*?)\)\s*;\s*$")


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


def qt_types(decl: str, unknown: list) -> set[str]:
    """A Qt declaration fragment -> the SET of LIDL types it can stand for."""
    t = decl.strip()
    t = re.sub(r"^const\s+", "", t)
    t = re.sub(r"[\s&*]+$", "", t).strip()
    t = norm(t)
    for qt, lidl in QT_TO_LIDL.items():
        if norm(qt) == t:
            return lidl
    unknown.append(decl.strip())
    return {f"<unknown:{decl.strip()}>"}


def parse_qt(path: Path, unknown: list):
    """A Qt provider header: only `LOGOS_METHOD`-marked declarations count.

    Returns methods as {name: ([set-per-param], set-for-return)} and NO events
    — see the module docstring for why a Qt provider has none to read.
    """
    methods = {}
    for line in path.read_text().splitlines():
        m = _QT_DECL.match(line)
        if not m:
            continue
        ret, name, params = m.groups()
        ptypes = []
        for p in split_params(params):
            frag = re.sub(r"\s+\w+$", "", p.strip())
            ptypes.append(qt_types(frag, unknown))
        methods[name] = (ptypes, qt_types(ret, unknown))
    return methods, {}


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
    kinds = {}
    for label, (path, kind) in COPIES.items():
        if not path.is_file():
            print(f"MISSING copy: {label} -> {path}")
            return 1
        kinds[label] = kind
        if kind == "lidl":
            parsed[label] = parse_lidl(path)
        elif kind == "cpp":
            parsed[label] = parse_cpp(path, unknown)
        elif kind == "cpp-qt":
            parsed[label] = parse_qt(path, unknown)
        else:
            print(f"unknown parse kind for {label}: {kind}")
            return 1

    if unknown:
        print("C++/Qt spellings not in the translation tables — add them to "
              "CPP_TO_LIDL / QT_TO_LIDL rather than letting the check skip a type:")
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
        qt = kinds[label] == "cpp-qt"
        for name, sig in ref_methods.items():
            if name not in methods:
                problems.append(f"{label}: missing method `{name}` "
                                f"(the shared interface declares it)")
            elif not qt and methods[name] != sig:
                problems.append(
                    f"{label}: method `{name}` is {methods[name]}, "
                    f"the shared interface says {sig}")
            elif qt:
                # Compatibility, not equality — one Qt spelling can stand for
                # several LIDL types (see QT_TO_LIDL).
                got_params, got_ret = methods[name]
                ref_params, ref_ret = sig
                if len(got_params) != len(ref_params):
                    problems.append(
                        f"{label}: method `{name}` takes {len(got_params)} "
                        f"parameters, the shared interface says {len(ref_params)}")
                elif any(r not in g for g, r in zip(got_params, ref_params)) \
                        or ref_ret not in got_ret:
                    problems.append(
                        f"{label}: method `{name}` is "
                        f"{[sorted(g) for g in got_params]} -> {sorted(got_ret)}, "
                        f"which cannot stand for the shared interface's "
                        f"{ref_params} -> {ref_ret}")
        if qt:
            # Events deliberately not compared for a Qt provider — the compiler
            # covers them. Skip the reference event loop entirely rather than
            # reporting 15 phantom "missing event"s.
            continue
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
