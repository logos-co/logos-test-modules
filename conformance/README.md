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
backend's `typeSupported()` gate *used to* reject records and `[bstr]` by name,
and its impl-header parser skipped `struct` entirely — so a header-first C++
provider could not even *declare* a record, and adding those types to `full_api`
would have broken `test_fullapi_cpp`'s build rather than adding a test.

They therefore live in `full_api_ext`. logos-cpp-sdk#125 lifted the gate — the
C++ ext provider is header-first too, declares its records as C++ structs, and
`[bstr]` is expressible on both sides — so the split is now a scope decision
(one frozen contract per table) rather than a backend limit.

That table runs **both** providers and carries a provider differential like
`full_api` does. The gap that remains there is the CONSUMER axis: one point
(`py`), no proxy.

The table and the registry live HERE, with the providers they describe. The
drivers live with the client each one uses — the `py` driver is
`logos-logoscore-py/conformance/run_matrix.py` (that repo already depends on
this one; the reverse would be a cycle).

## Running it

```bash
# from logos-logoscore-py, or via its `conformance-matrix` flake check
python3 conformance/run_matrix.py \
  --cases    <logos-test-modules>/conformance/cases.json \
  --known    <logos-test-modules>/conformance/known.json \
  --contract <logos-test-modules>/test-fullapi-proxy-module-rust/full_api.lidl \
  --cpp-modules  <test_fullapi_cpp.install>/modules \
  --rust-modules <test_fullapi_rust.install>/modules \
  --proxy-consumer qtproxy-sync=test_fullapi_qtproxy=<install>/modules=sync \
  --proxy-consumer qtproxy-async=test_fullapi_qtproxy=<install>/modules=async \
  --jsonl  matrix.jsonl \
  --report matrix.html \
  --md     known-broken.md
```

The two `--proxy-consumer` points are what make the consumer axis exist; without
them the run measures `py` alone and no consumer differential is computed.

Exit status is non-zero if any cell is `fail`, `xpass`, `uncovered`,
`dead-skip` (a `skip[]` pattern that matches no case in the table),
`skip-passes` (a cell declared unsupported on a surface that in fact answers
it), or `setup-failed` (a consumer that could not be pointed at a provider).
The last three exist because each one is a way for the run to look green while
measuring less than it claims.

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

Measured on consumer(s) `py`, `qtproxy-async`, `qtproxy-sync` against provider(s) `test_fullapi_cpp`, `test_fullapi_rust`. See `known.json` for the full measurements and evidence.

| id | cells in this run | cases | what |
|----|----|----|----|
| M4-residual | 6 | `adversarial/any/pending-call-canonical` | a CANONICAL-shape forgery of the deferred-call sentinel still hijacks a call |
| M3 | 4 | `adversarial/{tstr:any}/_bytes-key` | a one-key `_bytes` map reaching a Qt-typed map slot is reinterpreted as bytes and arrives EMPTY |
| Q1 | 12 | `hostile/[uint]/negative-element`<br>`hostile/[uint]/fractional-element`<br>`hostile/[int]/fractional-element` | a typed-numeric-array element is still not validated by a Qt-typed provider: the C++ signature spells [uint] and [any] alike as QVariantList, so array-ness is the whole of the declared type at that layer |
| Q1b | 4 | `hostile/[any]/scalar`<br>`hostile/{tstr:any}/scalar` | the Qt proxy can no longer reproduce the C++ cdylib provider's LENIENT answer for a non-array/non-object in a [any]/{tstr:any} slot, because its own dispatch refuses the argument before forwarding it |

### Closed

Kept because it explains why several green cases exist at all: they are the regression guards a fix left behind.

| id | fixed by | what |
|----|----|----|
| M1 / M1b / M5 | logos-protocol 362b03f — one canonical LIDL <-> JSON codec (#29), pinned here via logos-cpp-sdk 3d322bd / logos-module-builder / logos-logoscore-cli | a uint64 above int64max degrading to a double once nested (M1/M1b), and a bstr nested in a container being UTF-8 mangled (M5). Both were the same root cause: the Qt and plain-wire paths each had their own conversion, and neither preserved a value whose type the container did not declare. |
| M6 | logos-protocol — setEventListenerStdBridge now parses with nlohmann and converts via logos::nlohmannArgsToQVariantList, the same helper callMethodStdBridge uses | a uint64 above int64max degrading to a double on the EVENT path (uintEvent(2^64-1) -> 1.8446744073709552e+19) while the method path was exact. |
| bytes-on-the-event-path | the same change | canonical tagged bytes {"_bytes": ...} were not decoded by the event bridge — they arrived as a QVariantMap where the method path yields a QByteArray. |
| M2 | logos-qt-sdk cdylib glue (kVoidMethods) + logos-rust-sdk void arms | a `void` return answered `true` from the C++ provider and METHOD_FAILED from the Rust one. |
| M4 | logos-protocol — logos::isPendingCallSentinel, replacing a bare contains() at all four detection sites | a user map that merely CARRIED the sentinel key was taken for a deferred call. |

### Not measurable by this matrix

| id | why |
|----|----|
| M3-history | M3 is no longer unmeasurable — it moved to `xfail` with a coordinate. This is the record of why it sat here, because the reason is a reusable lesson about what a matrix can and cannot see. |

The table above is GENERATED. `run_matrix.py --md` writes it, the flake
check drops it at `$out/known-broken.md`, and it is pasted here verbatim.
It was hand-maintained until it drifted to eleven rows against a registry
of four — listing M1/M1b, M2, M4, M5/E1, E2, E3 and OPT1 as current long
after each moved to `fixed[]`, while omitting the one live entry with the
most cells. Regenerate rather than edit:

```bash
nix build .#checks.<system>.conformance-matrix   # in logos-logoscore-py
cp result/known-broken.md conformance/README.md  # the section body
```

### Cases written before the implementation

The `Optional/` family was written contract-first: the expectations were
derived from the LIDL contract before any generator read `?T`, so at the time
they were red on purpose — the point of writing them first is that the
implementation has a target to hit rather than a behaviour to bless. That
landed. Both ext providers now implement optionality, all 34 Optional cells
are measured against them, and the only one still registered is `OPT2`.
`known-ext.json` keeps the record under `fixed[].OPT1`.

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
on every cell — 0 deltas across 68 method cases x 2 providers. The difference
between `_result.toT()` and `qvariant_cast<T>(v)` is real in the generated
source and is now driven; it does not currently change an answer. What the Qt
surface changes is this, and none of it is mode-dependent:

| what | cells (per Qt consumer: cases x 2 providers) | where it happens |
|------|--------------------------------------------|------------------|
| **M3** — a one-key `_bytes` map into a typed map slot arrives `{}` | 1 x 2 = 2 | `nlohmannToQVariant`; invisible on an `any` slot, where the transformation is its own inverse |
| **Q1** — a typed-array element is not validated | 3 x 2 = 6 | the Qt *signature*, which has no element type to check against — see below |
| **Q1b** — `[any]`/`{tstr:any}` given a scalar, C++ provider only | 2 x 1 = 2 | the Qt dispatch now refuses what the C++ cdylib provider still accepts |
| M4-residual | 1 x 2 = 2 | pre-existing, identical on every consumer |

It was 22. Ten of them were **Q1**'s scalar/container shapes; they closed when
both Qt provider sites stopped coercing arguments and started decoding them
through the canonical codec. Two more did not close, they INVERTED — Q1b — and
the count above is the measured one, not the predicted one. The rule they now apply is the codec's own, which matters
because a naive "reject anything inexact" gets it wrong: a whole-valued `3.0` is
a legal integer (JSON does not distinguish it from `3`, and this CLI produces it
for the spelling `3.0`) while `3.7` is not.

All of them were attributed by replaying the same table through the **lp** proxy,
which is a hop of the same shape on a non-Qt client: it answered
`dispatch_failed` on all nine hostile cases and preserves the `_bytes` map. So
the extra hop was never the cause — the Qt api style was.

Note what the Qt surface **cannot** express, so a cell that looks green there is
read correctly: `[any]`, `[int]`, `[uint]`, `[float64]` and `[bool]` are all
`QVariantList`, and a `void` return becomes `QVariant(true)` in the generated
provider dispatch. `check_contract_copies.py` encodes exactly that collapse for
the `qtproxy-h` copy rather than pretending the mapping is 1:1.

That collapse is now also the whole of what Q1 has left. A provider can only
check an argument against the type it can *see*, and the Qt spelling of a typed
numeric array has already thrown the element type away by the time either Qt
dispatch reads it — which is why the six shapes whose type survives closed and
these three did not.
