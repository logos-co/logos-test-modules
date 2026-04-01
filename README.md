# logos-test-modules

Test modules for the Logos platform. These modules exercise **every API type and
combination** exposed by `logos-cpp-sdk`, organised into complementary modules
and a standalone thread-safety test suite:

| Module | Purpose |
|--------|---------|
| **test_basic_module** | Standalone module (no external libs, no IPC). Covers every supported parameter type, return type, argument count (0–5), LogosResult patterns, and events. |
| **test_extlib_module** | Wraps an external C library (`libstrutil`). Validates the external-library build pipeline. |
| **test_ipc_module** | Calls the two modules above via `LogosAPI`. Validates inter-module communication, generated type-safe wrappers, and event subscriptions. |
| **test_ipc_new_api_module** | Same as test_ipc_module but uses the new provider API (`LogosProviderBase` + `LOGOS_METHOD`). No `QObject` inheritance in the implementation class. |
| **test_dummy_module** | Minimal `LogosProviderBase` module (`noop()` only). Used as a binary template for the thread-safety tests — patched at the binary level to generate unique module copies. |

## SDK coverage matrix

### Parameter types (`toScopedQArgs`)

| Type | Tested in |
|------|-----------|
| `QString` | test_basic_module, test_extlib_module, test_ipc_module |
| `int` | test_basic_module |
| `bool` | test_basic_module |
| `QStringList` | test_basic_module |
| `QByteArray` | test_basic_module |
| `QUrl` | test_basic_module |

### Return types (`callRemoteMethod`)

| Type | Tested in |
|------|-----------|
| `void` | test_basic_module |
| `bool` | test_basic_module |
| `int` | test_basic_module, test_extlib_module |
| `QString` | test_basic_module, test_extlib_module, test_ipc_module |
| `LogosResult` | test_basic_module, test_ipc_module |
| `QVariant` | test_basic_module |
| `QJsonArray` | test_basic_module |
| `QStringList` | test_basic_module |

### Argument counts (0–5)

| Count | Method |
|-------|--------|
| 0 | `noArgs()` |
| 1 | `oneArg(QString)` |
| 2 | `twoArgs(QString, int)` |
| 3 | `threeArgs(QString, int, bool)` |
| 4 | `fourArgs(QString, int, bool, QString)` |
| 5 | `fiveArgs(QString, int, bool, QString, int)` |

### Inter-module communication

| Pattern | Tested in |
|---------|-----------|
| `LogosAPI::getClient` + `invokeRemoteMethod` | test_ipc_module |
| Generated `LogosModules` wrappers | test_ipc_module |
| Event subscription (`onEvent`) | test_ipc_module |
| Event emission (`eventResponse`) | test_basic_module, test_ipc_module |
| Cross-module chaining | test_ipc_module |

## Running tests

The integration test suite exercises the core modules (`test_basic_module`, `test_extlib_module`, `test_ipc_module`, `test_ipc_new_api_module`) via `logoscore`:

```bash
# From logos-test-modules
nix build .#tests -L

# From the workspace root
ws test logos-test-modules
```

### Running specific test groups

Use `TEST_GROUPS` to run a subset of tests. Available groups: `basic`, `extlib`, `ipc`, `ipc-new-api`, `multi`, `errors`, `unit`, `unit-new-api`.

```bash
# IPC tests only (standalone)
nix build .#checks.aarch64-darwin.ipc-tests -L    # macOS ARM
nix build .#checks.x86_64-linux.ipc-tests -L      # Linux

# IPC new-API tests (LogosProviderBase path)
nix build .#checks.aarch64-darwin.ipc-new-api-tests -L    # macOS ARM
nix build .#checks.x86_64-linux.ipc-new-api-tests -L      # Linux
```

### Unit tests (mock-based)

Unit tests use the SDK's mock transport layer — no real IPC or `logoscore` needed.
They verify that module methods call the expected inter-module APIs with the correct
arguments and handle return values properly.

```bash
# Standalone (from the logos-test-modules repo)
nix build .#checks.x86_64-linux.unit-tests -L       # Linux
nix build .#checks.aarch64-darwin.unit-tests -L      # macOS ARM
```

> **Temporary note — running from the workspace with local `logos-cpp-sdk` changes:**
>
> ```bash
> nix build 'path:./repos/logos-test-modules#checks.aarch64-darwin.unit-tests' -L \
>   --override-input logos-module-builder/logos-cpp-sdk path:./repos/logos-cpp-sdk
> ```
>
> (Only `logos-module-builder/logos-cpp-sdk` is needed — there is no direct `logos-cpp-sdk` input.)

### Unit tests — new provider API (mock-based)

Unit tests for the new provider API (`LogosProviderBase` + `LOGOS_METHOD`). Same mock
transport as above — no real IPC or `logoscore` needed.

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

- **Process** — `logos_core_process_plugin` concurrently on disjoint and shared module sets
- **Query under writes** — `logos_core_get_known_plugins` / `logos_core_get_loaded_plugins` called by reader threads while writers are processing or loading
- **Load** — `logos_core_load_plugin` and `logos_core_load_plugin_with_dependencies` on disjoint and shared sets, including unknown-name fast-failure paths
- **Unload** — `logos_core_unload_plugin` interleaved with concurrent load threads on a shared small module set

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
nix build .#test_ipc_module
nix build .#test_ipc_new_api_module
nix build .#test_dummy_module
```

## Manual testing with logoscore

```bash
# Build and test a single module
nix build .#test_basic_module -o result-basic
logoscore -m ./result-basic/lib -l test_basic_module \
  -c "test_basic_module.echo(hello)" \
  -c "test_basic_module.addInts(3, 4)"

# Test extlib module
nix build .#test_extlib_module -o result-extlib
logoscore -m ./result-extlib/lib -l test_extlib_module \
  -c "test_extlib_module.reverseString(hello)" \
  -c "test_extlib_module.uppercaseString(hello)"
```
