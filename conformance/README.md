# LIDL conformance matrix

One question per cell: **does a value of LIDL type `T`, in position `P`, survive
provider `R` → consumer `K` intact?**

The contract being checked is the type contract: one LIDL type maps to exactly
one type per language, numbers are 64-bit only, and bytes ride the canonical
`{"_bytes": "<base64url>"}` tag at every depth.

```
conformance/
  cases.json                the `full_api` case table — language-neutral,
                            shared by every driver
  known.json                its xfail registry: known-broken cells, with evidence
  ext-cases.json            the `full_api_ext` table — records, bytes at depth,
                            typed maps, nested composites
  known-ext.json            its xfail registry
  check_contract_copies.py  asserts the EIGHT hand-maintained copies of the
                            `full_api` contract agree
```

## Two contracts

`full_api` is implemented by **both** providers, and the C++ one is
header-first: its contract is derived from an impl header. The C++ cdylib
backend's `typeSupported()` gate rejects records and `[bstr]` by name, and its
impl-header parser skips `struct` entirely — so a header-first C++ provider
cannot even *declare* a record. Adding those types to `full_api` would not add a
test, it would break `test_fullapi_cpp`'s build.

They therefore live in `full_api_ext`, Rust-first. That table has **one**
provider today and so no differential column; that is a gap, not a design
choice, and it closes when a C++ ext provider exists.

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

The same comparison runs on the **consumer** axis, over every pair of consumer
surfaces per provider. That is not a symmetry for its own sake — it is where the
two most recent findings came from (`M3`, `Q1` in `known.json`), because a value
that survives one consumer's decode can be destroyed by another's.

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
| M3 | `_bytes` key collision | a user map with a single `_bytes` key is indistinguishable from a tagged byte string and is silently reinterpreted as one. Measurable since the Qt consumer landed: `echoMap({"_bytes":"aGk"})` arrives `{}`. On an `any` slot it stays invisible — the transformation is its own inverse there. |
| Q1 | hostile argument coerced | the generated Qt dispatch converts a JSON argument into the declared parameter type before the method body runs, so `echoUint(-1)` becomes `18446744073709551615` where every other surface answers `dispatch_failed`. Argument validation is not something a Qt-typed module can rely on. |
| M4 | `__logos_pending_call__` key collision | same class, worse outcome: a user map carrying that key hijacks the call. |
| M5 / E1 | `bstr` nested in a container | exact as a top-level scalar; UTF-8 mangled the moment it is nested — every byte ≥ 0x80 becomes U+FFFD. Exactly what the canonical tag exists to prevent, defeated one level down. |
| E2 | empty `bstr` nested in a container | arrives as `null` and fails the call. Dropped rather than corrupted — the louder of the two failure modes. |
| E3 | M1 through a record field | `expected integer at arg0.n, got number`; the field-path diagnostic is pinned on its own. |

## Other drivers

`logos-logoscore-py/conformance/run_matrix.py` runs the whole consumer axis in
one process — it has to, because the consumer differential only exists if the
surfaces are measured together. A consumer point is either the direct client
(`py`) or a forwarding module:

```
--proxy-consumer qtproxy-sync=test_fullapi_qtproxy=$DIR=sync
--proxy-consumer qtproxy-async=test_fullapi_qtproxy=$DIR=async
```

The proxy is loaded, pointed at each provider with `useProvider`, and — for a Qt
proxy — put in a call mode with `useCallMode`. Every setting is **read back**,
and a call mode is additionally proved by `lastCallStatus()` reporting
`ok-sync`/`ok-async` on a known-good call: a `useProvider` that silently no-ops
would make both providers measure identically, and a `useCallMode` that no-ops
would make the async half of the matrix a duplicate of the sync half. Neither
failure announces itself.

Still unwired: the QML bridge (the `skip[]` entries describe that surface), the
Rust/cdylib proxy, and the ext table, which has no proxy at all.

## Consumer surfaces

A proxy only adds a consumer *coordinate* if it reaches a different generated
client. Two of the three do not, which was measured before this table existed —
replaying the whole case table through the universal and cdylib proxies moved 2
cells out of 86.

| module | metadata | generated client | reaches |
|--------|----------|------------------|---------|
| `test_fullapi_proxy` | `interface: universal` | apiStyle=**lp** (Qt-free, `logos::LpClient`) | the C ABI / plain wire |
| `test_fullapi_proxy_rust` | `interface: cdylib` | the **Rust** client | the Rust decode |
| `test_fullapi_qtproxy` | `type: core`, **no `interface` key** | apiStyle=**qt** (`LogosAPIClient`, `QByteArray` / `qulonglong` / `QVariantList` / `LogosResult`) | `logos_json_convert.cpp` and the generated sync/async return tables |

The Qt one is the only surface that reaches the `_bytes` reinterpretation in
`nlohmannToQVariant` (registry entry **M3**), and the only one that drives the
generated ASYNC return table at all. `useCallMode sync|async` routes every
forwarded method through one table or the other, so the whole case table replays
twice; `syncProbe()` and `probeAsync()`/`getAsyncProbe()` remain as a fixed
type-tagged rendering of 18 calls, which the forwarding path cannot show because
the value is re-encoded on the way back.

**Measured, so it is not re-argued:** over the whole table the two tables agree
on every cell — 0 deltas across 67 method cases x 2 providers. The difference
between `_result.toT()` and `qvariant_cast<T>(v)` is real in the generated
source and is now driven; it does not currently change an answer. What the Qt
surface *does* change is 22 cells, and none of them are mode-dependent:

| what | cells (per Qt consumer: cases x 2 providers) | where it happens |
|------|--------------------------------------------|------------------|
| **M3** — a one-key `_bytes` map into a typed map slot arrives `{}` | 1 x 2 = 2 | `nlohmannToQVariant`; invisible on an `any` slot, where the transformation is its own inverse |
| **Q1** — a hostile argument is coerced instead of rejected | 9 x 2 = 18 | the generated Qt dispatch, before the provider sees the value |
| M4-residual | 1 x 2 = 2 | pre-existing, identical on every consumer |

Both were attributed by replaying the same table through the **lp** proxy, which
is a hop of the same shape on a non-Qt client: it answers `dispatch_failed` on
all nine hostile cases and preserves the `_bytes` map. So the extra hop is not
the cause — the Qt api style is.

Note what the Qt surface **cannot** express, so a cell that looks green there is
read correctly: `[any]`, `[int]`, `[uint]`, `[float64]` and `[bool]` are all
`QVariantList`, and a `void` return becomes `QVariant(true)` in the generated
provider dispatch. `check_contract_copies.py` encodes exactly that collapse for
the `qtproxy-h` copy rather than pretending the mapping is 1:1.
