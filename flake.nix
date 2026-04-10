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
      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in {
          test_basic_module = basic.packages.${system}.default;
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
          extlibInstall = extlib.packages.${system}.install;
          ipcInstall = ipc.packages.${system}.install;
          ipcNewApiInstall = ipc-new-api.packages.${system}.install;

          logoscorePkg = logos-logoscore-cli.packages.${system}.default;
          logosSdkPkg = logos-liblogos.inputs.logos-cpp-sdk.packages.${system}.default;
          logosLiblogosPkg = logos-liblogos.packages.${system}.default;

          # Merge all installed modules into a single directory
          modulesDir = pkgs.runCommand "test-modules-dir" {} ''
            mkdir -p $out

            for installed in ${basicInstall} ${extlibInstall} ${ipcInstall} ${ipcNewApiInstall}; do
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
                  logosSdkPkg    # provides logos-cpp-generator + SDK lib/headers
                ];

                buildInputs = [
                  pkgs.qt6.qtbase
                  pkgs.qt6.qtremoteobjects
                ];

                env = {
                  LOGOS_CPP_SDK_ROOT = "${logosSdkPkg}";
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
                ];

                buildInputs = [
                  pkgs.qt6.qtbase
                  pkgs.qt6.qtremoteobjects
                ];

                env = {
                  LOGOS_CPP_SDK_ROOT = "${logosSdkPkg}";
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
