{
  description = "Logos Test Modules — comprehensive SDK test suite (basic, extlib, IPC)";

  inputs = {
    logos-nix.url = "github:logos-co/logos-nix";
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    logos-liblogos.url = "github:logos-co/logos-liblogos";
    logos-logoscore-cli.url = "github:logos-co/logos-logoscore-cli";
    nixpkgs.follows = "logos-nix/nixpkgs";
  };

  outputs = { self, logos-nix, logos-module-builder, logos-liblogos, logos-logoscore-cli, nixpkgs }:
    let
      mkModule = logos-module-builder.lib.mkLogosModule;
      mkQmlModule = logos-module-builder.lib.mkLogosQmlModule;

      basic = mkModule {
        src = ./test-basic-module;
        configFile = ./test-basic-module/metadata.json;
      };

      # Pure-C++ mirror of `basic`. Same method matrix, but every signature
      # uses std::string / LogosMap / LogosList / StdLogosResult instead of
      # Qt types. The builder detects `interface: "universal"` in metadata
      # and runs `logos-cpp-generator --from-header` to produce the Qt glue.
      basicCpp = mkModule {
        src = ./test-basic-module-cpp;
        configFile = ./test-basic-module-cpp/metadata.json;
      };

      # ── full_api test chain ──────────────────────────────────────────────
      # Universal C++ provider covering EVERY supported method param/return
      # type and event param type. Reference impl of the `full_api` contract;
      # mirrored 1:1 by the Rust provider so a consumer can bind either.
      fullapiCpp = mkModule {
        src = ./test-fullapi-module-cpp;
        configFile = ./test-fullapi-module-cpp/metadata.json;
      };

      # Rust (cdylib) provider implementing the SAME full_api surface as
      # fullapiCpp — proves cross-language ABI + type parity. mkLogosModule is
      # language-agnostic; the metadata `codegen.rust` block drives the Rust path.
      fullapiRust = mkModule {
        src = ./test-fullapi-module-rust;
        configFile = ./test-fullapi-module-rust/metadata.json;
      };

      # The composite tail of the conformance matrix: records, bytes at depth,
      # typed maps, nested composites. A SEPARATE contract from full_api because
      # the C++ cdylib gate USED TO reject several of these types by name —
      # putting them in full_api would have broken test_fullapi_cpp's build
      # rather than testing anything. logos-cpp-sdk#125 lifted that gate; the
      # contracts stay split because each one is frozen, not because a backend
      # cannot express it.
      # C++ mirror of fullapiExtRust — HEADER-FIRST, like test_fullapi_cpp: its
      # metadata carries `interface: universal` and no `codegen` block, and the
      # contract is derived from src/test_fullapi_ext_cpp_impl.h, records and
      # all. Gives the ext table a second provider and therefore a differential.
      fullapiExtCpp = mkModule {
        src = ./test-fullapi-ext-module-cpp;
        configFile = ./test-fullapi-ext-module-cpp/metadata.json;
      };

      fullapiExtRust = mkModule {
        src = ./test-fullapi-ext-module-rust;
        configFile = ./test-fullapi-ext-module-rust/metadata.json;
      };

      # Universal C++ proxy: consumes the full_api surface of either provider via
      # an interface dependency (interfaces/full_api.h) and re-exposes it. Depends
      # on both providers so the host loads them and modules() is wired.
      fullapiProxy = mkModule {
        src = ./test-fullapi-proxy-module-cpp;
        configFile = ./test-fullapi-proxy-module-cpp/metadata.json;
        flakeInputs = {
          test_fullapi_cpp = fullapiCpp;
          test_fullapi_rust = fullapiRust;
        };
      };

      # Rust mirror of the proxy — consumes full_api via an interface dependency
      # (full_api.lidl -> FullApiClient::bind) and re-exposes it. Proves the
      # consumer/proxy pattern works cross-language in Rust too.
      fullapiProxyRust = mkModule {
        src = ./test-fullapi-proxy-module-rust;
        configFile = ./test-fullapi-proxy-module-rust/metadata.json;
        flakeInputs = {
          test_fullapi_cpp = fullapiCpp;
          test_fullapi_rust = fullapiRust;
        };
      };

      # QT-TYPED consumer/proxy — the matrix's third consumer surface. `type:
      # core` with NO `interface` key, so the builder picks apiStyle=qt and the
      # generated `bind_full_api(name)` wrapper is Qt-typed (QByteArray /
      # qulonglong / QVariantList / LogosResult). The two existing proxies both
      # bypass those wrappers (universal -> lp, cdylib -> Rust client), so this is
      # the only module that reaches logos_json_convert.cpp's `_bytes`
      # reinterpretation and the generated ASYNC return table.
      #
      # Built TWICE from the SAME `src`. The two builds differ in one thing: which
      # generator emitted the Qt-typed consumer wrapper the module calls its
      # provider through.
      #
      #   qtConsumerCodegen = "legacy"  logos-cpp-generator --general-only --api-style qt
      #   qtConsumerCodegen = "veneer"  logos-qt-generator --backend consumer
      #
      # Not one line of test-fullapi-qtproxy-module/src changes between them, so a
      # cell that differs is a difference between the two IMPLEMENTATIONS of the Qt
      # surface and cannot be a difference in how the module was written. That is
      # the whole claim being measured.
      mkQtProxy = { qtConsumerCodegen ? "legacy" }: mkModule {
        src = ./test-fullapi-qtproxy-module;
        configFile = ./test-fullapi-qtproxy-module/metadata.json;
        flakeInputs = {
          test_fullapi_cpp = fullapiCpp;
          test_fullapi_rust = fullapiRust;
        };
        preConfigure = ''
          echo "Running logos-cpp-generator --provider-header for test_fullapi_qtproxy..."
          logos-cpp-generator --provider-header "$(pwd)/src/test_fullapi_qtproxy_impl.h" --output-dir "$(pwd)"
          if [ ! -f logos_provider_dispatch.cpp ]; then
            echo "ERROR: logos_provider_dispatch.cpp was not generated" >&2
            exit 1
          fi
        '' + (if qtConsumerCodegen == "veneer" then ''
          # Re-emit full_api_api.{h,cpp} from the SAME contract with the veneer
          # backend, over the legacy generator's output. Same file names, same
          # class, same ctor — `logos_sdk.h`'s `bind_full_api` and every call site
          # in src/ bind to it unchanged.
          echo "Re-emitting the Qt consumer wrapper via logos-qt-generator --backend consumer..."
          _veneer_dir=$(mktemp -d)
          logos-qt-generator --lidl "$(pwd)/interfaces/full_api.lidl" \
            --backend consumer --module full_api --class FullApi --bind bound \
            --output-dir "$_veneer_dir"
          for f in full_api_api.h full_api_api.cpp; do
            if [ ! -s "$_veneer_dir/$f" ]; then
              echo "ERROR: logos-qt-generator did not emit $f" >&2
              exit 1
            fi
          done
          # buildPlugin moved the legacy .h into generated_code/include and copied
          # the .cpp there; logos_sdk.cpp is compiled from generated_code/ and
          # #includes "full_api_api.cpp" beside itself. Replace every copy so no
          # stale legacy text can be picked up by either include path.
          cp -f "$_veneer_dir/full_api_api.cpp" ./generated_code/full_api_api.cpp
          cp -f "$_veneer_dir/full_api_api.h"   ./generated_code/include/full_api_api.h
          cp -f "$_veneer_dir/full_api_api.cpp" ./generated_code/include/full_api_api.cpp
          rm -f ./generated_code/full_api_api.h
          grep -q "logos::qt::LpBridge" ./generated_code/include/full_api_api.h \
            || { echo "ERROR: the wrapper in generated_code is not the veneer" >&2; exit 1; }
        '' else "");
      };

      fullapiQtProxy = mkQtProxy { };
      # Same sources, veneer codegen. Same module name — load one or the other.
      fullapiQtProxyVeneerCodegen = mkQtProxy { qtConsumerCodegen = "veneer"; };

      # Lifecycle smoke test for LogosModuleContext. The impl inherits the
      # SDK base class and exposes:
      #   (a) the four context accessors through plain methods so the
      #       integration runner can call them via logoscore and assert
      #       they match the persistence-path the host provisioned;
      #   (b) cross-module callBasicEcho / callBasicAddInts that exercise
      #       the typed `modules().test_basic_module.<method>`
      #       chain end-to-end, proving the codegen-emitted onInit
      #       constructed a real LogosModules and threaded it through
      #       the context base.
      # See test-context-module-cpp/src/*.h for the full rationale.
      contextCpp = mkModule {
        src = ./test-context-module-cpp;
        configFile = ./test-context-module-cpp/metadata.json;
        flakeInputs = {
          test_basic_module = basic;
          # test_basic_module_cpp is the universal-typed twin of basic;
          # contextCpp depends on it specifically to exercise the typed
          # `onTestEvent` / `onMultiArgEvent` accessors generated from
          # its `logos_events:` block. Being `interface: universal`,
          # basicCpp publishes a `lidl` contract, so the codegen is
          # driven by `--dep test_basic_module_cpp=<its .lidl>` and its
          # plugin is never built for contextCpp. (`basic` publishes no
          # `lidl`, so IT takes the legacy header-copy path instead —
          # contextCpp is lp-typed, so it copies basic's `headers-lp`.)
          test_basic_module_cpp = basicCpp;
        };
      };

      # Dependency-interface smoke test. Declares a LOCAL `basic_calc`
      # interface (interfaces/basic_calc.h, a subset of test_basic_module_cpp)
      # and binds it to a module name at runtime via
      # modules().bind_basic_calc(name). Depends on test_basic_module_cpp so
      # the host loads it and modules() is wired; the interface names no
      # concrete module. Exercises the bound wrapper's sync + event accessors.
      interfaceCpp = mkModule {
        src = ./test-interface-module-cpp;
        configFile = ./test-interface-module-cpp/metadata.json;
        flakeInputs = {
          test_basic_module_cpp = basicCpp;
        };
      };

      extlib = mkModule {
        src = ./test-extlib-module;
        configFile = ./test-extlib-module/metadata.json;
      };

      ipc = mkModule {
        src = ./test-ipc-module;
        configFile = ./test-ipc-module/metadata.json;
        flakeInputs = {
          test_basic_module = basic;
          test_extlib_module = extlib;
        };
      };

      dummy = mkModule {
        src = ./test-dummy-module;
        configFile = ./test-dummy-module/metadata.json;
        preConfigure = ''
          echo "Running logos-cpp-generator --provider-header for dummy_module_000000..."
          logos-cpp-generator --provider-header "$(pwd)/src/dummy_module_000000_impl.h" --output-dir "$(pwd)"
          if [ ! -f logos_provider_dispatch.cpp ]; then
            echo "ERROR: logos_provider_dispatch.cpp was not generated" >&2
            exit 1
          fi
        '';
      };

      ipc-new-api = mkModule {
        src = ./test-ipc-module-new-api;
        configFile = ./test-ipc-module-new-api/metadata.json;
        flakeInputs = {
          test_basic_module = basic;
          test_extlib_module = extlib;
        };
        preConfigure = ''
          # Run provider-header code generation for the new-API module
          echo "Running logos-cpp-generator --provider-header for test_ipc_new_api_module..."
          logos-cpp-generator --provider-header "$(pwd)/src/test_ipc_new_api_impl.h" --output-dir "$(pwd)"
          if [ ! -f logos_provider_dispatch.cpp ]; then
            echo "ERROR: logos_provider_dispatch.cpp was not generated" >&2
            exit 1
          fi
        '';
      };

      # Universal QML+Qt UI plugin consuming full_api via an interface
      # dependency. mkQmlModule delegates to the same buildCppPlugin pipeline
      # (universal codegen + interface deps) and bundles the QML view.
      # A SECOND Qt-typed consumer of full_api, written independently of the
      # qtproxy and binding a `.h` INTERFACE rather than a .lidl contract — the
      # path where the void/QVariant type-mapper defect lives. Built both ways
      # for the same reason: nothing under src/ changes between them.
      mkFullapiUi = { qtConsumerCodegen ? "legacy" }: mkQmlModule {
        src = ./test-fullapi-ui-module;
        configFile = ./test-fullapi-ui-module/metadata.json;
        flakeInputs = {
          test_fullapi_proxy = fullapiProxy;
        };
        preConfigure = if qtConsumerCodegen != "veneer" then "" else ''
          echo "Re-emitting the Qt consumer wrapper via logos-qt-generator --backend consumer (from .h)..."
          _veneer_dir=$(mktemp -d)
          logos-qt-generator --from-header "$(pwd)/interfaces/full_api.h" --impl-class IFullApi \
            --metadata "$(pwd)/metadata.json" \
            --backend consumer --module full_api --class FullApi --bind bound \
            --output-dir "$_veneer_dir"
          for f in full_api_api.h full_api_api.cpp; do
            if [ ! -s "$_veneer_dir/$f" ]; then
              echo "ERROR: logos-qt-generator did not emit $f" >&2
              exit 1
            fi
          done
          cp -f "$_veneer_dir/full_api_api.cpp" ./generated_code/full_api_api.cpp
          cp -f "$_veneer_dir/full_api_api.h"   ./generated_code/include/full_api_api.h
          cp -f "$_veneer_dir/full_api_api.cpp" ./generated_code/include/full_api_api.cpp
          grep -q "logos::qt::LpBridge" ./generated_code/include/full_api_api.h \
            || { echo "ERROR: the wrapper in generated_code is not the veneer" >&2; exit 1; }
        '';
      };

      fullapiUi = mkFullapiUi { };
      fullapiUiVeneerCodegen = mkFullapiUi { qtConsumerCodegen = "veneer"; };

      # QML-only variant: no C++ backend, consumes test_fullapi_cpp directly
      # from QML via the `logos` bridge (logos.callModule / onModuleEvent).
      fullapiUiQml = mkQmlModule {
        src = ./test-fullapi-ui-qml-module;
        configFile = ./test-fullapi-ui-qml-module/metadata.json;
        flakeInputs = {
          test_fullapi_cpp = fullapiCpp;
        };
      };

      # THROWAWAY PROBE: `type: ui_qml` + `interface: universal`, forwarding the
      # eight methods behind the nine hostile-argument cases to test_fullapi_cpp.
      # Exists to settle by BUILDING and RUNNING which dispatch a universal
      # ui_qml module gets — the nix `apiStyleCmakeFlags` conditional excludes
      # ui_qml from the lp selection while `autoCodegen` routes it to
      # `--backend ui`, so the two axes were suspected to disagree.
      uiqmlProbe = mkQmlModule {
        src = ./test-uiqml-probe-module;
        configFile = ./test-uiqml-probe-module/metadata.json;
        flakeInputs = {
          test_fullapi_cpp = fullapiCpp;
        };
      };

      qmlOnly = mkQmlModule {
        src = ./test-qml-only-module;
        configFile = ./test-qml-only-module/metadata.json;
      };

      qmlBackend = mkQmlModule {
        src = ./test-qml-backend-module;
        configFile = ./test-qml-backend-module/metadata.json;
        flakeInputs = {
          test_basic_module = basic;
        };
      };

      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = fn: nixpkgs.lib.genAttrs systems fn;
    in {
      # Forwarded full module outputs — each entry exposes every variant
      # produced by mkLogosModule (.default / .install / .lib / .include /
      # .lgx / .api-lgx), so downstream consumers can pick whichever output
      # fits their use case (e.g. `.install` for a logoscore modulesDir,
      # `.include` for generated API headers, `.lgx` for a package archive).
      modules = forAllSystems (system: {
        test_basic_module = basic.packages.${system};
        test_basic_module_cpp = basicCpp.packages.${system};
        test_fullapi_cpp = fullapiCpp.packages.${system};
        test_fullapi_rust = fullapiRust.packages.${system};
        test_fullapi_ext_rust = fullapiExtRust.packages.${system};
        test_fullapi_ext_cpp = fullapiExtCpp.packages.${system};
        test_fullapi_proxy = fullapiProxy.packages.${system};
        test_fullapi_proxy_rust = fullapiProxyRust.packages.${system};
        test_fullapi_qtproxy = fullapiQtProxy.packages.${system};
        test_fullapi_qtproxy_veneercodegen = fullapiQtProxyVeneerCodegen.packages.${system};
        test_fullapi_ui = fullapiUi.packages.${system};
        test_fullapi_ui_veneercodegen = fullapiUiVeneerCodegen.packages.${system};
        test_fullapi_ui_qml = fullapiUiQml.packages.${system};
        test_uiqml_probe = uiqmlProbe.packages.${system};
        test_context_module_cpp = contextCpp.packages.${system};
        test_interface_module_cpp = interfaceCpp.packages.${system};
        test_extlib_module = extlib.packages.${system};
        test_ipc_module = ipc.packages.${system};
        test_ipc_new_api_module = ipc-new-api.packages.${system};
        test_dummy_module = dummy.packages.${system};
        test_qml_only = qmlOnly.packages.${system};
        test_qml_backend = qmlBackend.packages.${system};
      });

      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in {
          test_basic_module = basic.packages.${system}.default;
          test_basic_module_cpp = basicCpp.packages.${system}.default;
          test_fullapi_cpp = fullapiCpp.packages.${system}.default;
          test_fullapi_rust = fullapiRust.packages.${system}.default;
          test_fullapi_ext_rust = fullapiExtRust.packages.${system}.default;
          test_fullapi_ext_cpp = fullapiExtCpp.packages.${system}.default;
          test_fullapi_proxy = fullapiProxy.packages.${system}.default;
          test_fullapi_proxy_rust = fullapiProxyRust.packages.${system}.default;
          test_fullapi_qtproxy = fullapiQtProxy.packages.${system}.default;
          test_fullapi_ui = fullapiUi.packages.${system}.default;
          test_fullapi_ui_qml = fullapiUiQml.packages.${system}.default;
          test_uiqml_probe = uiqmlProbe.packages.${system}.default;
          test_context_module_cpp = contextCpp.packages.${system}.default;
          test_interface_module_cpp = interfaceCpp.packages.${system}.default;
          test_extlib_module = extlib.packages.${system}.default;
          test_ipc_module = ipc.packages.${system}.default;
          test_ipc_new_api_module = ipc-new-api.packages.${system}.default;
          test_dummy_module = dummy.packages.${system}.default;
 	  test_qml_only = qmlOnly.packages.${system}.default;
          test_qml_backend = qmlBackend.packages.${system}.default;

          # Convenience alias: `nix build .#tests` runs the integration test suite
          tests = self.checks.${system}.tests;

          default = pkgs.symlinkJoin {
            name = "logos-test-modules";
            paths = [
              basic.packages.${system}.default
              basicCpp.packages.${system}.default
              contextCpp.packages.${system}.default
              extlib.packages.${system}.default
              ipc.packages.${system}.default
              ipc-new-api.packages.${system}.default
              dummy.packages.${system}.default
            ];
          };
        }
      );

      checks = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };

          # Use the install outputs (bundle + lgpm install in one step)
          basicInstall = basic.packages.${system}.install;
          basicCppInstall = basicCpp.packages.${system}.install;
          contextCppInstall = contextCpp.packages.${system}.install;
          extlibInstall = extlib.packages.${system}.install;
          ipcInstall = ipc.packages.${system}.install;
          ipcNewApiInstall = ipc-new-api.packages.${system}.install;
          fullapiCppInstall = fullapiCpp.packages.${system}.install;
          fullapiRustInstall = fullapiRust.packages.${system}.install;
          fullapiProxyInstall = fullapiProxy.packages.${system}.install;
          fullapiProxyRustInstall = fullapiProxyRust.packages.${system}.install;

          logoscorePkg = logos-logoscore-cli.packages.${system}.default;
          # The SDK triple comes from logos-module-builder, NOT logos-liblogos.
          #
          # These headers compile code the BUILDER's generator emitted, so they
          # have to be the builder's own SDK or the two drift: reaching through
          # logos-liblogos.inputs gave a cpp-sdk pinned by a different repo on a
          # different schedule, and the moment the builder's generator started
          # emitting `#include "logos_async_result.h"` (logos-cpp-sdk#132) the
          # unit tests stopped compiling — the generator was new, the headers
          # were old, and nothing in either repo was wrong on its own.
          #
          # Generator and headers now move together by construction.
          logosSdkPkg = logos-module-builder.inputs.logos-cpp-sdk.packages.${system}.default;
          logosQtSdkPkg = logos-module-builder.inputs.logos-qt-sdk.packages.${system}.default;
          logosProtocolPkg = logos-module-builder.inputs.logos-protocol.packages.${system}.default;
          logosLiblogosPkg = logos-liblogos.packages.${system}.default;

          # Merge all installed modules into a single directory
          modulesDir = pkgs.runCommand "test-modules-dir" {} ''
            mkdir -p $out

            for installed in ${basicInstall} ${basicCppInstall} ${contextCppInstall} ${extlibInstall} ${ipcInstall} ${ipcNewApiInstall} ${fullapiCppInstall} ${fullapiRustInstall} ${fullapiProxyInstall} ${fullapiProxyRustInstall}; do
              if [ -d "$installed/modules" ]; then
                cp -rn "$installed/modules/." "$out/"

              fi
            done

            echo "Installed modules:"
            ls -la $out/
          '';
        in {
          tests = pkgs.runCommand "logos-test-modules-tests" {
            nativeBuildInputs = [
              logoscorePkg
              pkgs.jq
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
          } ''
            export QT_QPA_PLATFORM=offscreen
            export QT_FORCE_STDERR_LOGGING=1
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
            ''}
            mkdir -p $out

            echo "Module directories:"
            ls -la ${modulesDir}/
            echo ""

            echo "Running logos-test-modules integration tests..."
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${modulesDir} \
              2>&1 | tee $out/test-results.txt

            echo "Tests completed successfully."
          '';

          # Full-API chain integration: exercises the fullapi provider + proxy
          # (C++ and Rust) under logoscore — every method type incl. typed-scalar
          # arrays via the proxy's probeArrays, and event round-trips.
          fullapi-tests = pkgs.runCommand "logos-test-modules-fullapi-tests" {
            nativeBuildInputs = [
              logoscorePkg
              pkgs.jq
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
          } ''
            export QT_QPA_PLATFORM=offscreen
            export QT_FORCE_STDERR_LOGGING=1
            export TEST_GROUPS=fullapi
            export TEST_TIMEOUT=30
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
            ''}
            mkdir -p $out
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${modulesDir} \
              2>&1 | tee $out/fullapi-results.txt
            echo "Full-API tests completed."
          '';

          # QML module build + packaging verification
          # Uses the install outputs from mkLogosQmlModule (lgpm is run internally
          # by nix-bundle-logos-module-install, no direct lgpm dependency needed).
          qml-modules = pkgs.runCommand "logos-test-qml-modules" {
            nativeBuildInputs = [ pkgs.jq ];
          } ''
            set -euo pipefail
            echo "=== QML Module Tests ==="

            # --- QML-only module ---
            echo "Testing QML-only module build..."
            defaultPkg="${qmlOnly.packages.${system}.default}"
            test -f "$defaultPkg/Main.qml"
            echo "PASS: QML-only default has Main.qml"

            test -f "$defaultPkg/metadata.json"
            echo "PASS: QML-only default has metadata.json"

            type=$(jq -r '.type' "$defaultPkg/metadata.json")
            test "$type" = "ui_qml"
            echo "PASS: QML-only type is ui_qml"

            view=$(jq -r '.view' "$defaultPkg/metadata.json")
            test "$view" = "Main.qml"
            echo "PASS: QML-only view is Main.qml"

            # Verify LGX package exists
            qmlOnlyLgx="${qmlOnly.packages.${system}.lgx}"
            test -f "$qmlOnlyLgx"/*.lgx
            echo "PASS: QML-only LGX package exists"

            # Verify install output (produced by nix-bundle-logos-module-install)
            qmlOnlyInstall="${qmlOnly.packages.${system}.install}"
            manifest=$(find "$qmlOnlyInstall" -name "manifest.json" | head -1)
            test -n "$manifest"
            echo "PASS: QML-only install has manifest.json"

            mtype=$(jq -r '.type' "$manifest")
            test "$mtype" = "ui_qml"
            echo "PASS: installed manifest type is ui_qml"

            mview=$(jq -r '.view' "$manifest")
            test "$mview" = "Main.qml"
            echo "PASS: installed manifest has view field"

            # --- QML + backend module ---
            echo ""
            echo "Testing QML + backend module build..."
            backendDefault="${qmlBackend.packages.${system}.default}"
            test -d "$backendDefault/lib"
            echo "PASS: backend default has lib/ directory"

            test -f "$backendDefault/lib/metadata.json"
            echo "PASS: backend lib/ has metadata.json"

            # Check for plugin .so/.dylib
            pluginCount=$(find "$backendDefault/lib" -name "test_qml_backend_plugin.*" | wc -l)
            test "$pluginCount" -gt 0
            echo "PASS: backend plugin library exists"

            # Check for replica factory
            factoryCount=$(find "$backendDefault/lib" -name "test_qml_backend_replica_factory.*" | wc -l)
            test "$factoryCount" -gt 0
            echo "PASS: replica factory plugin exists"

            # Check QML view is bundled
            test -f "$backendDefault/lib/qml/Main.qml"
            echo "PASS: QML view bundled in lib/qml/"

            # Verify LGX package exists
            backendLgx="${qmlBackend.packages.${system}.lgx}"
            test -f "$backendLgx"/*.lgx
            echo "PASS: backend LGX package exists"

            # Verify install output
            backendInstall="${qmlBackend.packages.${system}.install}"
            bmanifest=$(find "$backendInstall" -name "manifest.json" | head -1)
            test -n "$bmanifest"
            echo "PASS: backend install has manifest.json"

            btype=$(jq -r '.type' "$bmanifest")
            test "$btype" = "ui_qml"
            echo "PASS: backend manifest type is ui_qml"

            bview=$(jq -r '.view' "$bmanifest")
            test "$bview" = "qml/Main.qml"
            echo "PASS: backend manifest has view field"

            bmain=$(jq '.main | length' "$bmanifest")
            test "$bmain" -gt 0
            echo "PASS: backend manifest has main entries"

            echo ""
            echo "All QML module tests passed."
            mkdir -p $out
            echo "passed" > $out/results.txt
          '';

          # NOTE: QML backend → core module IPC tests require logos-standalone-app
          # (ui-host process) which is not available in the headless test environment.
          # The backend plugin is tested structurally (build, LGX, manifest) by
          # qml-modules above. Runtime IPC is verified manually via:
          #   ws run logos-standalone-app --local ... -l test_qml_backend

          # Async-only tests (validates invokeRemoteMethodAsync + generated wrappers)
          async-tests = pkgs.runCommand "logos-test-modules-async-tests" {
            nativeBuildInputs = [
              logoscorePkg
              pkgs.jq
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
          } ''
            export QT_QPA_PLATFORM=offscreen
            export QT_FORCE_STDERR_LOGGING=1
            export TEST_GROUPS=async
            export TEST_TIMEOUT=30
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
            ''}
            mkdir -p $out

            echo "Running async-only tests..."
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${modulesDir} \
              2>&1 | tee $out/test-results.txt

            echo "Async tests completed."
          '';

          # IPC-only tests (faster iteration on inter-module communication)
          ipc-tests = pkgs.runCommand "logos-test-modules-ipc-tests" {
            nativeBuildInputs = [
              logoscorePkg
              pkgs.jq
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
          } ''
            export QT_QPA_PLATFORM=offscreen
            export QT_FORCE_STDERR_LOGGING=1
            export TEST_GROUPS=ipc
            export TEST_TIMEOUT=30
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
            ''}
            mkdir -p $out

            echo "Running IPC-only tests..."
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${modulesDir} \
              2>&1 | tee $out/test-results.txt

            echo "IPC tests completed."
          '';

          # IPC new-API tests (LogosProviderBase path)
          ipc-new-api-tests = pkgs.runCommand "logos-test-modules-ipc-new-api-tests" {
            nativeBuildInputs = [
              logoscorePkg
              pkgs.jq
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
          } ''
            export QT_QPA_PLATFORM=offscreen
            export QT_FORCE_STDERR_LOGGING=1
            export TEST_GROUPS=ipc-new-api
            export TEST_TIMEOUT=30
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
            ''}
            mkdir -p $out

            echo "Running IPC new-API tests..."
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${modulesDir} \
              2>&1 | tee $out/test-results.txt

            echo "IPC new-API tests completed."
          '';

          # Unit tests using the mock transport — no real IPC / logoscore required
          unit-tests =
            let
              basicInclude = basic.packages.${system}.include;
              extlibInclude = extlib.packages.${system}.include;

              # Build the test executable via CMake
              testBin = pkgs.stdenv.mkDerivation {
                pname = "test-ipc-module-unit-tests";
                version = "1.0.0";

                src = ./test-ipc-module;

                nativeBuildInputs = [
                  pkgs.cmake
                  pkgs.ninja
                  pkgs.qt6.wrapQtAppsNoGuiHook
                  logosSdkPkg    # provides logos-cpp-generator + SDK headers
                  logosQtSdkPkg
                  logosProtocolPkg
                ];

                buildInputs = [
                  pkgs.qt6.qtbase
                  pkgs.qt6.qtremoteobjects
                ];

                env = {
                  LOGOS_CPP_SDK_ROOT = "${logosSdkPkg}";
                  LOGOS_QT_SDK_ROOT = "${logosQtSdkPkg}";
                  LOGOS_PROTOCOL_ROOT = "${logosProtocolPkg}";
                  LOGOS_LIBLOGOS_ROOT = "${logosLiblogosPkg}";
                };

                dontUseCmakeConfigure = true;

                buildPhase = ''
                  runHook preBuild

                  # Generate logos_sdk.cpp (general mode)
                  mkdir -p generated_code
                  cat > metadata.json <<'METADATA_EOF'
                  {
                    "name": "test_ipc_module",
                    "version": "1.0.0",
                    "type": "core",
                    "category": "testing",
                    "description": "Test module exercising inter-module communication via LogosAPI",
                    "dependencies": ["test_basic_module", "test_extlib_module"]
                  }
                  METADATA_EOF
                  logos-cpp-generator --metadata metadata.json --general-only --output-dir ./generated_code

                  # Copy dependency-generated API headers alongside the umbrella headers
                  cp ${basicInclude}/include/*.h ./generated_code/ 2>/dev/null || true
                  cp ${basicInclude}/include/*.cpp ./generated_code/ 2>/dev/null || true
                  cp ${extlibInclude}/include/*.h ./generated_code/ 2>/dev/null || true
                  cp ${extlibInclude}/include/*.cpp ./generated_code/ 2>/dev/null || true

                  # MOC needs metadata.json next to the plugin header for Q_PLUGIN_METADATA
                  cp metadata.json src/metadata.json

                  # CMake configure + build (out-of-source, pointing at tests/ subdir)
                  mkdir -p build && cd build
                  cmake ../tests -GNinja \
                    -DLOGOS_CPP_SDK_ROOT=${logosSdkPkg} \
                    -DLOGOS_QT_SDK_ROOT=${logosQtSdkPkg} \
                    -DLOGOS_PROTOCOL_ROOT=${logosProtocolPkg} \
                    -DLOGOS_LIBLOGOS_ROOT=${logosLiblogosPkg}
                  ninja test_ipc_module_tests

                  runHook postBuild
                '';

                installPhase = ''
                  runHook preInstall
                  mkdir -p $out/bin
                  cp test_ipc_module_tests $out/bin/
                  runHook postInstall
                '';
              };
            in
            pkgs.runCommand "logos-test-modules-unit-tests" {
              nativeBuildInputs = [ testBin ]
                ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
            } ''
              export QT_QPA_PLATFORM=offscreen
              export QT_FORCE_STDERR_LOGGING=1
              export TEST_GROUPS=unit
              ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
                export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
              ''}
              mkdir -p $out

              export UNIT_TEST_BIN="${testBin}/bin/test_ipc_module_tests"
              bash ${./tests/run_tests.sh} \
                ${logoscorePkg}/bin/logoscore \
                ${modulesDir} \
                2>&1 | tee $out/unit-test-results.txt

              echo "Unit tests completed."
            '';

          # Unit tests for the new provider API (mock transport, no logoscore)
          unit-tests-new-api =
            let
              basicInclude = basic.packages.${system}.include;
              extlibInclude = extlib.packages.${system}.include;

              testBinNewApi = pkgs.stdenv.mkDerivation {
                pname = "test-ipc-new-api-module-unit-tests";
                version = "1.0.0";

                src = ./test-ipc-module-new-api;

                nativeBuildInputs = [
                  pkgs.cmake
                  pkgs.ninja
                  pkgs.qt6.wrapQtAppsNoGuiHook
                  logosSdkPkg
                  logosQtSdkPkg
                  logosProtocolPkg
                ];

                buildInputs = [
                  pkgs.qt6.qtbase
                  pkgs.qt6.qtremoteobjects
                ];

                env = {
                  LOGOS_CPP_SDK_ROOT = "${logosSdkPkg}";
                  LOGOS_QT_SDK_ROOT = "${logosQtSdkPkg}";
                  LOGOS_PROTOCOL_ROOT = "${logosProtocolPkg}";
                  LOGOS_LIBLOGOS_ROOT = "${logosLiblogosPkg}";
                };

                dontUseCmakeConfigure = true;

                buildPhase = ''
                  runHook preBuild

                  # Generate logos_sdk.cpp (general mode — for LogosModules wrappers)
                  mkdir -p generated_code
                  cat > metadata.json <<'METADATA_EOF'
                  {
                    "name": "test_ipc_new_api_module",
                    "version": "1.0.0",
                    "type": "core",
                    "category": "testing",
                    "description": "Test module exercising the new provider API (LogosProviderBase)",
                    "dependencies": ["test_basic_module", "test_extlib_module"]
                  }
                  METADATA_EOF
                  logos-cpp-generator --metadata metadata.json --general-only --output-dir ./generated_code

                  # Copy dependency-generated API headers
                  cp ${basicInclude}/include/*.h ./generated_code/ 2>/dev/null || true
                  cp ${basicInclude}/include/*.cpp ./generated_code/ 2>/dev/null || true
                  cp ${extlibInclude}/include/*.h ./generated_code/ 2>/dev/null || true
                  cp ${extlibInclude}/include/*.cpp ./generated_code/ 2>/dev/null || true

                  # Generate provider dispatch code (callMethod/getMethods)
                  logos-cpp-generator --provider-header "$(pwd)/src/test_ipc_new_api_impl.h" --output-dir "$(pwd)"
                  echo "Generated provider dispatch:"
                  ls -la logos_provider_dispatch.cpp

                  # MOC needs metadata.json next to the loader header
                  cp metadata.json src/metadata.json

                  # CMake configure + build
                  mkdir -p build && cd build
                  cmake ../tests -GNinja \
                    -DLOGOS_CPP_SDK_ROOT=${logosSdkPkg} \
                    -DLOGOS_QT_SDK_ROOT=${logosQtSdkPkg} \
                    -DLOGOS_PROTOCOL_ROOT=${logosProtocolPkg} \
                    -DLOGOS_LIBLOGOS_ROOT=${logosLiblogosPkg}
                  ninja test_ipc_new_api_module_tests

                  runHook postBuild
                '';

                installPhase = ''
                  runHook preInstall
                  mkdir -p $out/bin
                  cp test_ipc_new_api_module_tests $out/bin/
                  runHook postInstall
                '';
              };
            in
            pkgs.runCommand "logos-test-modules-unit-tests-new-api" {
              nativeBuildInputs = [ testBinNewApi ]
                ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
            } ''
              export QT_QPA_PLATFORM=offscreen
              export QT_FORCE_STDERR_LOGGING=1
              export TEST_GROUPS=unit-new-api
              export UNIT_NEW_API_TEST_BIN="${testBinNewApi}/bin/test_ipc_new_api_module_tests"
              ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
                export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
              ''}
              mkdir -p $out

              bash ${./tests/run_tests.sh} \
                ${logoscorePkg}/bin/logoscore \
                ${modulesDir} \
                2>&1 | tee $out/unit-test-results-new-api.txt

              echo "New-API unit tests completed."
            '';

          # Thread safety tests — exercises PluginManager / PluginRegistry under concurrency.
          # Uses the dummy module as a real Qt plugin binary template.
          thread-safety-tests =
            let
              dummyLibPkg = dummy.packages.${system}.lib;

              testBin = pkgs.stdenv.mkDerivation {
                pname = "thread-safety-tests";
                version = "1.0.0";

                src = ./test-thread-safety;

                nativeBuildInputs = [
                  pkgs.cmake
                  pkgs.ninja
                  pkgs.qt6.wrapQtAppsNoGuiHook
                ] ++ pkgs.lib.optionals pkgs.stdenv.isDarwin [ pkgs.darwin.cctools ]
                  ++ pkgs.lib.optionals pkgs.stdenv.isLinux  [ pkgs.patchelf ];

                buildInputs = [
                  pkgs.qt6.qtbase
                  pkgs.qt6.qtremoteobjects
                  pkgs.gtest
                  logosLiblogosPkg
                ];

                cmakeFlags = [
                  "-GNinja"
                  "-DCMAKE_BUILD_TYPE=Release"
                  "-DLOGOS_LIBLOGOS_ROOT=${logosLiblogosPkg}"
                  "-DDUMMY_PLUGIN_TEMPLATE_DIR=${dummyLibPkg}/lib"
                  "-DCMAKE_BUILD_WITH_INSTALL_RPATH=TRUE"
                  "-DCMAKE_INSTALL_RPATH=${logosLiblogosPkg}/lib"
                ];

                installPhase = ''
                  runHook preInstall
                  mkdir -p $out/bin
                  cp thread_safety_tests $out/bin/
                  runHook postInstall
                '';
              };
            in
            pkgs.runCommand "logos-thread-safety-tests" {
              nativeBuildInputs = [ testBin logosLiblogosPkg ]
                ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
            } ''
              export QT_QPA_PLATFORM=offscreen
              export QT_FORCE_STDERR_LOGGING=1
              ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
                export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
              ''}
              export DUMMY_PLUGIN_TEMPLATE_DIR="${dummyLibPkg}/lib"
              export LOGOS_HOST_PATH="${logosLiblogosPkg}/bin/logos_host"
              mkdir -p $out
              echo "Running thread safety tests..."
              ${testBin}/bin/thread_safety_tests --gtest_output=xml:$out/test-results.xml
              echo "Thread safety tests completed."
            '';
        }
      );
    };
}
