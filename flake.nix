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

          # Convenience alias: `nix build .#tests` runs the integration test suite
          tests = self.checks.${system}.tests;

          default = pkgs.symlinkJoin {
            name = "logos-test-modules";
            paths = [
              basic.packages.${system}.default
              extlib.packages.${system}.default
              ipc.packages.${system}.default
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
          logoscorePkg = logos-liblogos.packages.${system}.logos-liblogos-bin;

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

          # Combined modules directory with all three
          allModulesDir = pkgs.symlinkJoin {
            name = "test-modules-dir";
            paths = [ basicDir extlibDir ipcDir ];
          };
        in {
          tests = pkgs.runCommand "logos-test-modules-tests" {
            nativeBuildInputs = [
              logoscorePkg basicPkg extlibPkg ipcPkg
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
            echo ""

            echo "Running logos-test-modules integration tests..."
            bash ${./tests/run_tests.sh} \
              ${logoscorePkg}/bin/logoscore \
              ${basicDir} \
              ${extlibDir} \
              ${allModulesDir} \
              2>&1 | tee $out/test-results.txt

            echo "Tests completed successfully."
          '';
        }
      );
    };
}
