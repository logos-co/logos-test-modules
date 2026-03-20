{
  description = "Logos Test Modules — comprehensive SDK test suite (basic, extlib, IPC)";

  inputs = {
    logos-nix.url = "github:logos-co/logos-nix";
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    logos-liblogos.url = "github:logos-co/logos-liblogos";
    logos-package-manager.url = "github:logos-co/logos-package-manager-module";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    nixpkgs.follows = "logos-nix/nixpkgs";
  };

  outputs = { self, logos-nix, logos-module-builder, logos-liblogos, logos-package-manager, nix-bundle-lgx, nixpkgs }:
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

          # Use the lib outputs (they carry src for nix-bundle-lgx metadata.json)
          basicLib = basic.packages.${system}.lib;
          extlibLib = extlib.packages.${system}.lib;
          ipcLib = ipc.packages.${system}.lib;

          logoscorePkg = logos-liblogos.packages.${system}.logos-liblogos-bin;
          lgpmCli = logos-package-manager.packages.${system}.cli;
          bundleLgx = nix-bundle-lgx.bundlers.${system}.default;

          # Bundle each test module as .lgx (dev variant, e.g. linux-amd64-dev)
          basicLgx = bundleLgx basicLib;
          extlibLgx = bundleLgx extlibLib;
          ipcLgx = bundleLgx ipcLib;

          # Install all .lgx packages into a single modules directory via lgpm
          modulesDir = pkgs.runCommand "test-modules-dir" {
            nativeBuildInputs = [ lgpmCli ];
          } ''
            mkdir -p $out

            for lgxFile in ${basicLgx}/*.lgx ${extlibLgx}/*.lgx ${ipcLgx}/*.lgx; do
              echo "Installing $(basename "$lgxFile")..."
              lgpm --modules-dir "$out" install --file "$lgxFile"
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
        }
      );
    };
}
