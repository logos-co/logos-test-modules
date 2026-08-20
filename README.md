# logos-test-modules

Test modules for the Logos platform. These modules exercise **every API type and
combination** exposed by `logos-cpp-sdk`, organised into complementary modules
and a standalone thread-safety test suite:

| Module | Purpose |
|--------|---------|
| **test_basic_module** | Standalone module (no external libs, no IPC). Covers every supported parameter type, return type, argument count (0–5), LogosResult patterns, and events. |
| **test_extlib_module** | Wraps an external C library (`libstrutil`). Validates the external-library build pipeline. |
| **test_ipc_new_api_module** | Calls the two modules above. Validates inter-module communication, generated type-safe wrappers (sync and async), cross-module chaining, and events — all from an `interface: "universal"` module with no Qt in its own translation units. |
| **test_dummy_module** | Minimal `interface: "universal"` module (`noop()` only) — its impl derives from `LogosModuleContext` (it was a hand-written `LogosProviderBase` Qt plugin before the universal migration). Used as a binary template for the thread-safety tests — patched at the binary level to generate unique module copies. |

## SDK coverage matrix

### Parameter types

Spelled below as the Qt types a Qt consumer sees. The impl headers declare the
std equivalents (`std::string`, `int64_t`, `bool`, `std::vector<std::string>`,
`std::vector<uint8_t>`); an inbound argument is decoded by the generated cdylib
glue through `logos::fromJson<T>`, which has the declared element type. (It used
to arrive via `toScopedQArgs`, the QMetaObject dispatch helper — that path still
exists in logos-plugin-qt but only serves `interface: "legacy"` Q_INVOKABLE
providers, and this repo no longer has one.)

| Type | Tested in |
|------|-----------|
| `QString` | test_basic_module, test_extlib_module, test_ipc_new_api_module |
| `qlonglong` | test_basic_module |
| `bool` | test_basic_module |
| `QStringList` | test_basic_module |
| `QByteArray` | test_basic_module |

### Return types (`callRemoteMethod`)

| Type | Tested in |
|------|-----------|
| `void` | test_basic_module |
| `bool` | test_basic_module |
| `qlonglong` | test_basic_module, test_extlib_module |
| `QString` | test_basic_module, test_extlib_module, test_ipc_new_api_module |
| `LogosResult` | test_basic_module, test_ipc_new_api_module |
| `QVariant` | test_basic_module |
| `QVariantList` | test_basic_module |
| `QStringList` | test_basic_module |

### Universal migration — the Qt types that moved

`test_basic_module` and `test_extlib_module` used to be hand-written Qt plugins.
Both now declare `interface: "universal"`: the impl class is plain C++ with no Qt
in it, that class IS the contract. `logos-cpp-generator --header-to-lidl` derives
the LIDL from its header, `logos-cpp-generator --lidl ... --backend cdylib` emits
the Qt-free C-ABI exports, and logos-plugin-qt's `logos-qt-host-generator --lidl
... --backend cdylib` emits the Qt plugin glue. (There is no `LOGOS_METHOD`
marker any more — a module's plain public methods ARE its API — and no
`logos_provider_dispatch.cpp`.)

A Qt-free header has no way to ask for a Qt-specific type, so three declared
types could not be carried across. The values on the wire are unchanged — only
the declared types move:

| Was (Qt plugin) | Is (universal) | Why |
|---|---|---|
| `int` | `qlonglong` | LIDL numbers are 64-bit; the only widening in the table |
| `QJsonArray` (`returnJsonArray`, `makeJsonArray`) | `QVariantList` | `LogosList` → LIDL `[any]`; still the same JSON array on the wire |
| `QUrl` param (`urlToString`) | `QString` param | LIDL `tstr`; callers were serialising the URL to its string form anyway |

`QUrl` no longer appears anywhere in the suite, so the SDK's `QUrl` parameter
conversion is no longer covered by these fixtures. `urlToString` keeps its
*behaviour* — it returns the URL with the case-insensitive parts (scheme, host)
lowercased — but it now does that normalisation itself instead of borrowing
`QUrl`'s.

### Strings are UTF-8, and lengths are in characters

Every string these fixtures take or return is UTF-8, and every method that
counts, indexes or reorders "characters" means **one Unicode code point**:

| Method | Answer |
|---|---|
| `test_basic_module.stringLength("héllo")` | `5` |
| `test_basic_module.validateInput("héllo").value.length` | `5` |
| `test_extlib_module.countChars("héllo")` | `5` |
| `test_extlib_module.countChar("héllo", "é")` | `1` |
| `test_extlib_module.reverseString("héllo")` | `"olléh"` |

A character above the BMP (an emoji, say) counts as **1**. Neither of the two
answers this suite used to get is a length: the Qt modules answered
`QString::length()`, the number of UTF-16 units (2 for that emoji), and the
first universal port answered `std::string::size()`, the number of UTF-8 bytes
(4 for it, and 6 for `"héllo"`). Counting bytes is also why `reverseString` used
to produce a byte sequence that was not even valid UTF-8: in the universal port
the call failed outright at serialisation, and in the Qt module before it the
broken bytes were decoded into replacement characters. Every row above has a
regression assertion in the `basic` or `extlib` group of `tests/run_tests.sh`;
before those were added, every assertion touching these methods was ASCII, where
characters, UTF-16 units and bytes all agree.

`uppercaseString` / `lowercaseString` are the exception, and deliberately so:
they are ASCII-only case mappings performed by the external C library, and bytes
≥ 0x80 pass through untouched (`"héllo"` → `"HéLLO"`).

### Argument counts (0–5)

| Count | Method |
|-------|--------|
| 0 | `noArgs()` |
| 1 | `oneArg(QString)` |
| 2 | `twoArgs(QString, qlonglong)` |
| 3 | `threeArgs(QString, qlonglong, bool)` |
| 4 | `fourArgs(QString, qlonglong, bool, QString)` |
| 5 | `fiveArgs(QString, qlonglong, bool, QString, qlonglong)` |

### Inter-module communication

| Pattern | Tested in |
|---------|-----------|
| Generated `modules().<dep>` wrappers (sync) | test_ipc_new_api_module |
| Generated `<name>Async` wrappers over `lp_invoke_async` | test_ipc_new_api_module |
| Event subscription — generated `on<Event>(callback)` accessors | test_fullapi_proxy, test_fullapi_proxy_rust, test_fullapi_qtproxy |
| Event emission — typed `logos_events:` | test_basic_module, test_basic_module_cpp, test_ipc_new_api_module |
| Cross-module chaining | test_ipc_new_api_module |
| Qt-typed consumer (`consumer_api_style: "qt"`) | test_fullapi_qtproxy |

## Running tests

The integration test suite exercises the core modules (`test_basic_module`,
`test_basic_module_cpp`, `test_context_module_cpp`, `test_extlib_module`,
`test_ipc_new_api_module`, and the `full_api` provider/proxy chain) against a
long-lived `logoscore` daemon:

```bash
# From logos-test-modules
nix build .#tests -L

# From the workspace root
ws test logos-test-modules
```

### Running specific test groups

Use `TEST_GROUPS` to run a subset of tests. Available groups: `basic`,
`basic-cpp`, `context-cpp`, `extlib`, `fullapi`, `ipc-new-api`, `multi`,
`errors`, `unit-new-api`.

(The `ipc`, `async` and `unit` groups were removed in af567c1 together with
`test_ipc_module`, the last `interface: "legacy"` provider in the repo — its
surface is mirrored 1:1 by `test_ipc_new_api_module`, and the async assertions
moved to the `ipc-new-api` group before the module was deleted.)

Not every group has a dedicated flake check; the ones that do are
`ipc-new-api-tests`, `fullapi-tests`, `unit-tests-new-api`,
`thread-safety-tests` and `qml-modules`. `tests` runs every group.

```bash
# IPC new-API tests — the Qt-free `interface: "universal"` consumer
nix build .#checks.aarch64-darwin.ipc-new-api-tests -L    # macOS ARM
nix build .#checks.x86_64-linux.ipc-new-api-tests -L      # Linux

# full_api provider + proxy chain
nix build .#checks.aarch64-darwin.fullapi-tests -L        # macOS ARM
nix build .#checks.x86_64-linux.fullapi-tests -L          # Linux
```

### Unit tests (mock-based)

Unit tests use the SDK's mock transport layer — no real IPC or `logoscore` needed.
They verify that module methods call the expected inter-module APIs with the correct
arguments and handle return values properly.

There is exactly one such check now, `unit-tests-new-api` — see the next section
for how to run it. (A second one, `unit-tests`, built `test_ipc_module`'s Qt-typed
binary; the module and the check were both removed in af567c1, so
`.#checks.<system>.unit-tests` no longer exists.)

> **Temporary note — running from the workspace with local `logos-cpp-sdk` changes:**
>
> ```bash
> nix build 'path:./repos/logos-test-modules#checks.aarch64-darwin.unit-tests-new-api' -L \
>   --override-input logos-module-builder/logos-cpp-sdk path:./repos/logos-cpp-sdk
> ```
>
> (Only `logos-module-builder/logos-cpp-sdk` is needed — there is no direct `logos-cpp-sdk` input.)

### Unit tests — `test_ipc_new_api_module` (mock-based)

Unit tests for `test_ipc_new_api_module`, an `interface: "universal"` consumer:
its impl is a plain C++ class deriving from `LogosModuleContext` and the contract
is derived from its header by the generator. (It was written against
`LogosProviderBase` + the `LOGOS_METHOD` marker; both that base class as an
authoring surface and the marker are gone — a module's plain public methods are
now its API.) Same mock transport as above — no real IPC or `logoscore` needed.

```bash
# Standalone (from the logos-test-modules repo)
nix build .#checks.x86_64-linux.unit-tests-new-api -L       # Linux
nix build .#checks.aarch64-darwin.unit-tests-new-api -L      # macOS ARM
```

From the workspace root:

```bash
# Via workspace flake (propagates local overrides)
nix build ".#checks.aarch64-darwin.logos-test-modules--unit-tests-new-api" \
  --override-input logos-cpp-sdk path:./repos/logos-cpp-sdk \
  --override-input logos-liblogos path:./repos/logos-liblogos \
  --override-input logos-module-builder path:./repos/logos-module-builder \
  --override-input logos-test-modules path:./repos/logos-test-modules -L
```

### Thread-safety tests

Exercises `logos_core` under concurrent load using GTest.
The `test_dummy_module` binary is used as a template: 100 copies are generated by
patching the embedded plugin name at the binary level, giving each thread a distinct
real Qt plugin to work with.

All operations go through the public `logos_core` C API. Tests cover:

- **Process** — `logos_core_process_module` concurrently on disjoint and shared module sets
- **Query under writes** — `logos_core_get_known_modules` / `logos_core_get_loaded_modules` called by reader threads while writers are processing or loading
- **Load** — `logos_core_load_module(name, with_dependencies)` on disjoint and shared sets, with the flag both ways, including unknown-name fast-failure paths (there used to be a separate `logos_core_load_module_with_dependencies`; liblogos#130 merged it into the bool)
- **Unload** — `logos_core_unload_module(name, with_dependents)` interleaved with concurrent load threads on a shared small module set

```bash
# Standalone
nix build .#checks.aarch64-darwin.thread-safety-tests -L    # macOS ARM
nix build .#checks.x86_64-linux.thread-safety-tests -L      # Linux

# From workspace root
ws test logos-test-modules --auto-local
```

## Building

```bash
# All modules
nix build

# Individual modules
nix build .#test_basic_module
nix build .#test_extlib_module
nix build .#test_ipc_new_api_module
nix build .#test_dummy_module

# `nix flake show` lists the rest — the basic-cpp / context / interface
# fixtures, the full_api provider-proxy-UI chain, and the QML modules.
```

## Manual testing with logoscore

`logoscore` is daemon + client only: the inline mode (`-l <mods> -c
"<module>.<method>(args)"`) was removed in logos-logoscore-cli#41, so start a
daemon over a modules directory and drive it with the `call` client. This is
what `tests/run_tests.sh` does — its `dcall_inline` helper exists purely to
translate the old inline spelling into these subcommands so the pre-existing
substring assertions still match.

```bash
# Build and test a single module
nix build .#test_basic_module -o result-basic

CFG=$(mktemp -d)
logoscore -D --config-dir "$CFG" -m ./result-basic/lib &
logoscore --config-dir "$CFG" load-module test_basic_module
logoscore --config-dir "$CFG" call test_basic_module echo hello
logoscore --config-dir "$CFG" call test_basic_module addInts 3 4
logoscore --config-dir "$CFG" stop

# Test extlib module
nix build .#test_extlib_module -o result-extlib

CFG=$(mktemp -d)
logoscore -D --config-dir "$CFG" -m ./result-extlib/lib &
logoscore --config-dir "$CFG" load-module test_extlib_module
logoscore --config-dir "$CFG" call test_extlib_module reverseString hello
logoscore --config-dir "$CFG" call test_extlib_module uppercaseString hello
logoscore --config-dir "$CFG" stop
```
