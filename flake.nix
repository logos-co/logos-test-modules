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
          echo "Generated provider dispatch:"
          ls -la logos_provider_dispatch.cpp 2>/dev/null || echo "WARNING: dispatch file not found"
          cat logos_provider_dispatch.cpp 2>/dev/null || true
        '';
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

          # Convenience alias: `nix build .#tests` runs the integration test suite
          tests = self.checks.${system}.tests;

          default = pkgs.symlinkJoin {
            name = "logos-test-modules";
            paths = [
              basic.packages.${system}.default
              extlib.packages.${system}.default
              ipc.packages.${system}.default
              ipc-new-api.packages.${system}.default
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
        }
      );
    };
}
