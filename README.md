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

## Building

Each module has its own `flake.nix` and can be built independently:

```bash
# From the workspace root
nix build path:./repos/logos-test-modules/test-basic-module
nix build path:./repos/logos-test-modules/test-extlib-module
nix build path:./repos/logos-test-modules/test-ipc-module
```

## Testing with logoscore

```bash
# Build all three
nix build path:./repos/logos-test-modules/test-basic-module  -o result-basic
nix build path:./repos/logos-test-modules/test-extlib-module -o result-extlib
nix build path:./repos/logos-test-modules/test-ipc-module    -o result-ipc

# Test basic module (standalone)
logoscore -m ./result-basic/lib -l test_basic_module \
  -c "test_basic_module.noArgs()" \
  -c "test_basic_module.echo(hello)" \
  -c "test_basic_module.addInts(3, 4)" \
  -c "test_basic_module.successResult()"

# Test extlib module (standalone)
logoscore -m ./result-extlib/lib -l test_extlib_module \
  -c "test_extlib_module.reverseString(hello)" \
  -c "test_extlib_module.uppercaseString(hello)"

# Test IPC module (needs all three)
logoscore \
  -m ./result-basic/lib -m ./result-extlib/lib -m ./result-ipc/lib \
  -l test_ipc_module \
  -c "test_ipc_module.callBasicEcho(hello)" \
  -c "test_ipc_module.callExtlibReverse(world)"
```
