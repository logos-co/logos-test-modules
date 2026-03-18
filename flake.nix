{
  description = "Logos Test Modules — comprehensive SDK test suite (basic, extlib, IPC)";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    logos-liblogos.url = "github:logos-co/logos-liblogos";
    nixpkgs.follows = "logos-module-builder/nixpkgs";
  };

  outputs = { self, logos-module-builder, logos-liblogos, nixpkgs }:
    let
      mkModule = logos-module-builder.lib.mkLogosModule;

      basic = mkModule {
        src = ./test-basic-module;
        configFile = ./test-basic-module/module.yaml;
      };

      extlib = mkModule {
        src = ./test-extlib-module;
        configFile = ./test-extlib-module/module.yaml;
      };

      ipc = mkModule {
        src = ./test-ipc-module;
        configFile = ./test-ipc-module/module.yaml;
        moduleInputs = {
          test_basic_module = basic;
          test_extlib_module = extlib;
        };
      };

      ipc-new-api = mkModule {
        src = ./test-ipc-module-new-api;
        configFile = ./test-ipc-module-new-api/module.yaml;
        moduleInputs = {
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
          basicPkg = basic.packages.${system}.default;
          extlibPkg = extlib.packages.${system}.default;
          ipcPkg = ipc.packages.${system}.default;
          ipcNewApiPkg = ipc-new-api.packages.${system}.default;
          logoscorePkg = logos-liblogos.packages.${system}.logos-liblogos-bin;
          logosSdkPkg = logos-module-builder.inputs.logos-cpp-sdk.packages.${system}.default;
          logosLiblogosPkg = logos-module-builder.inputs.logos-liblogos.packages.${system}.default;

          # Determine the library extension for this platform
          libExt = if pkgs.stdenv.hostPlatform.isDarwin then "dylib" else "so";

          # Determine platform variant strings for manifest.json
          platformVariants = if pkgs.stdenv.hostPlatform.isDarwin then
            (if pkgs.stdenv.hostPlatform.isAarch64 then
              ''"darwin-arm64": "PLUGIN_FILE", "darwin-aarch64": "PLUGIN_FILE"''
            else
              ''"darwin-x86_64": "PLUGIN_FILE", "darwin-amd64": "PLUGIN_FILE"'')
          else
            (if pkgs.stdenv.hostPlatform.isAarch64 then
              ''"linux-aarch64": "PLUGIN_FILE", "linux-arm64": "PLUGIN_FILE"''
            else
              ''"linux-x86_64": "PLUGIN_FILE", "linux-amd64": "PLUGIN_FILE"'');

          # Create a properly structured modules directory with manifest.json
          # logoscore expects: modules-dir/module_name/manifest.json + plugin.so
          mkModuleDir = name: pkg: pkgs.runCommand "module-dir-${name}" {} ''
            mkdir -p $out/${name}
            pluginFile="${name}_plugin.${libExt}"

            # Create manifest.json
            variants='${platformVariants}'
            variants="''${variants//PLUGIN_FILE/$pluginFile}"
            cat > $out/${name}/manifest.json <<MANIFEST
            {
              "name": "${name}",
              "version": "1.0.0",
              "main": { $variants }
            }
            MANIFEST

            # Copy the plugin file
            cp ${pkg}/lib/${name}_plugin.${libExt} $out/${name}/
          '';

          basicDir = mkModuleDir "test_basic_module" basicPkg;
          extlibDir = mkModuleDir "test_extlib_module" extlibPkg;
          ipcDir = mkModuleDir "test_ipc_module" ipcPkg;
          ipcNewApiDir = mkModuleDir "test_ipc_new_api_module" ipcNewApiPkg;

          # Combined modules directory with all modules
          allModulesDir = pkgs.symlinkJoin {
            name = "test-modules-dir";
            paths = [ basicDir extlibDir ipcDir ipcNewApiDir ];
          };
        in {
          tests = pkgs.runCommand "logos-test-modules-tests" {
            nativeBuildInputs = [
              logoscorePkg basicPkg extlibPkg ipcPkg ipcNewApiPkg
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
          } ''
            export QT_QPA_PLATFORM=offscreen
            export QT_FORCE_STDERR_LOGGING=1
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
            ''}
            mkdir -p $out

            echo "Module directories:"
            ls -la ${basicDir}/test_basic_module/
            ls -la ${extlibDir}/test_extlib_module/
            ls -la ${ipcDir}/test_ipc_module/
            ls -la ${ipcNewApiDir}/test_ipc_new_api_module/
            echo ""

            export IPC_NEW_API_DIR="${ipcNewApiDir}"
            echo "Running logos-test-modules integration tests..."
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${basicDir} \
              ${extlibDir} \
              ${allModulesDir} \
              2>&1 | tee $out/test-results.txt

            echo "Tests completed successfully."
          '';

          # IPC-only tests (faster iteration on inter-module communication)
          ipc-tests = pkgs.runCommand "logos-test-modules-ipc-tests" {
            nativeBuildInputs = [
              logoscorePkg basicPkg extlibPkg ipcPkg ipcNewApiPkg
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
              ${basicDir} \
              ${extlibDir} \
              ${allModulesDir} \
              2>&1 | tee $out/test-results.txt

            echo "IPC tests completed."
          '';

          # IPC new-API tests (LogosProviderBase path)
          ipc-new-api-tests = pkgs.runCommand "logos-test-modules-ipc-new-api-tests" {
            nativeBuildInputs = [
              logoscorePkg basicPkg extlibPkg ipcNewApiPkg
            ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.qt6.qtbase ];
          } ''
            export QT_QPA_PLATFORM=offscreen
            export QT_FORCE_STDERR_LOGGING=1
            export TEST_GROUPS=ipc-new-api
            export TEST_TIMEOUT=30
            export IPC_NEW_API_DIR="${ipcNewApiDir}"
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
              export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/${pkgs.qt6.qtbase.qtPluginPrefix}"
            ''}
            mkdir -p $out

            echo "Running IPC new-API tests..."
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${basicDir} \
              ${extlibDir} \
              ${allModulesDir} \
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

              # run_tests.sh requires logoscore + module dirs as positional args even for the
              # unit group; pass dummy placeholders since they are not used when TEST_GROUPS=unit.
              bash ${./tests/run_tests.sh} \
                ${logoscorePkg}/bin/logoscore \
                ${basicDir} \
                ${extlibDir} \
                ${allModulesDir} \
                ${testBin}/bin/test_ipc_module_tests \
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
                ${basicDir} \
                ${extlibDir} \
                ${allModulesDir} \
                2>&1 | tee $out/unit-test-results-new-api.txt

              echo "New-API unit tests completed."
            '';
        }
      );
    };
}
