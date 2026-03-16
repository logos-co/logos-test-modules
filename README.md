# logos-test-modules

Test modules for the Logos platform. These modules exercise **every API type and
combination** exposed by `logos-cpp-sdk`, organised into three complementary
modules:

| Module | Purpose |
|--------|---------|
| **test_basic_module** | Standalone module (no external libs, no IPC). Covers every supported parameter type, return type, argument count (0–5), LogosResult patterns, and events. |
| **test_extlib_module** | Wraps an external C library (`libstrutil`). Validates the external-library build pipeline. |
| **test_ipc_module** | Calls the two modules above via `LogosAPI`. Validates inter-module communication, generated type-safe wrappers, and event subscriptions. |

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

The integration test suite exercises all three modules via `logoscore`:

```bash
# From logos-test-modules
nix build .#tests -L

# From the workspace root
ws test logos-test-modules
```

### Running specific test groups

Use `TEST_GROUPS` to run a subset of tests. Available groups: `basic`, `extlib`, `ipc`.

```bash
# IPC tests only (standalone)
nix build .#checks.aarch64-darwin.ipc-tests -L    # macOS ARM
nix build .#checks.x86_64-linux.ipc-tests -L      # Linux
```

## Building

```bash
# All modules
nix build

# Individual modules
nix build .#test_basic_module
nix build .#test_extlib_module
nix build .#test_ipc_module
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
