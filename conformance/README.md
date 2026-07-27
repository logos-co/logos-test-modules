# LIDL conformance matrix

One question per cell: **does a value of LIDL type `T`, in position `P`, survive
provider `R` → consumer `K` intact?**

The contract being checked is the type contract: one LIDL type maps to exactly
one type per language, numbers are 64-bit only, and bytes ride the canonical
`{"_bytes": "<base64url>"}` tag at every depth.

```
conformance/
  cases.json      the case table — language-neutral, shared by every driver
  known.json      the xfail registry: cells that are known broken, with evidence
```

The table and the registry live HERE, with the providers they describe. The
drivers live with the client each one uses — the `py` driver is
`logos-logoscore-py/conformance/run_matrix.py` (that repo already depends on
this one; the reverse would be a cycle).

## Running it

```bash
# from logos-logoscore-py, or via its `conformance-matrix` flake check
python3 conformance/run_matrix.py \
  --cpp-modules  <test_fullapi_cpp.install>/modules \
  --rust-modules <test_fullapi_rust.install>/modules \
  --contract     <logos-test-modules>/test-fullapi-proxy-module-rust/full_api.lidl \
  --jsonl        matrix.jsonl
```

Exit status is non-zero if any cell is `fail`, `xpass`, or `uncovered`.

## Reading the report

A red cell is a **coordinate**, never an aggregate token:

```
fail      [uint]/method_arg,method_return/test_fullapi_rust/py
          case=[uint]/boundary/max
          expected=[18446744073709551615]
          actual  =[1.8446744073709552e+19]
```

`type / position / provider / consumer`. That shape matters: the previous
arrangement asserted `ALL_OK` over the *combined* output of both providers, so
one provider alone could satisfy the assertion while the other was broken.

## What keeps it honest

**Differential comparison.** Every case runs against *both* providers and their
answers are compared to each other, independently of `expect`. This needs nobody
to know the right answer in advance, and it is how the `void` divergence was
found. A case whose providers legitimately differ declares that with
`expect_by_provider`; anything else disagreeing is a failure.

**`xfail` is a registry, not a deletion.** A known-broken cell lives in
`known.json` with its measurement and evidence. If it starts passing, the run
reports `xpass` and **fails** — so the registry gets updated when a fix lands
instead of the fix going unnoticed.

**Coverage is computed from the contract.** `--contract` parses the `.lidl` and
fails the run if any declared `(type, position)` has no case. This is the
structural guard against the state this replaced: a 31-method contract with six
assertions.

**Comparison is type-strict.** `same()` refuses `1 == True` and `1 == 1.0`. A
matrix that compares with `==` cannot see an integer degrading to a float, which
is most of what it exists to catch.

**Values, not shapes.** An assertion on `len(result)` is not coverage. The
drivers this replaces reported `intList=3 uintList=2`, which passes whatever the
elements became.

## Adding a case

Add a row to `cases.json` naming the cell it covers:

```jsonc
{ "id": "uint/boundary/max", "type": "uint", "position": "method_arg,method_return",
  "method": "echoUint", "args": [18446744073709551615], "expect": 18446744073709551615,
  "tags": ["boundary"], "why": "above int64max there is no signed representation" }
```

- **Bytes** are written `{"_bytes": "<base64url>"}` and materialized by the
  driver into its native byte type. `"_bytes": "__ALL_BYTES__"` expands to all
  256 byte values.
- **`"raw": true`** opts out of that materialization. It is what makes the
  adversarial cases expressible: "a *user* map that happens to have a single
  `_bytes` key must come back as a map". Without it the driver would convert
  both the argument and the expectation to bytes and the cell would compare
  bytes to bytes and pass no matter what the system did.
- **`expect_by_provider`** pins a divergence rather than papering over it.
- Adding a *type* costs one LIDL line, one impl method per provider, and N table
  rows — **zero per-consumer cost**. That property is the point: the failure
  mode being designed out is structural, not laziness.

## Current known-broken cells

See `known.json` for the measurements and evidence. In summary:

| id | cell | what |
|----|------|------|
| M1 / M1b | `uint` inside a container | a uint64 above int64max degrades to a double once nested; exact as a top-level scalar. The C++ provider *looks* green for `[uint]` because its typed decode coerces the double back — the Rust provider takes it untyped and reports the loss faithfully. |
| M2 | `void` return | the C++ provider answers JSON `true`; the Rust one fails the call. Pinned as `expect_by_provider`, so it shows in the report. |
| M3 | `_bytes` key collision | a user map with a single `_bytes` key is indistinguishable from a tagged byte string and is silently reinterpreted as one. |
| M4 | `__logos_pending_call__` key collision | same class, worse outcome: a user map carrying that key hijacks the call. |

## Other drivers

`logos-logoscore-py/conformance/run_matrix.py` is the `py` consumer (through the
`logoscore` CLI). The other
consumers replay the *same* `cases.json`: the C++ (`lp`/`std`/Qt) proxies, the
Rust proxy, and the QML bridge. Each needs a `runCases(json) -> json` entry
point rather than hand-written per-method checks; the report format is identical
apart from the `consumer` coordinate.
