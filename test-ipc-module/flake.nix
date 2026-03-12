{
  description = "Test IPC Module — exercises inter-module communication via LogosAPI";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nixpkgs.follows = "logos-module-builder/nixpkgs";

    test-basic-module = {
      url = "github:logos-co/logos-test-modules?dir=test-basic-module";
      inputs.logos-module-builder.follows = "logos-module-builder";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    test-extlib-module = {
      url = "github:logos-co/logos-test-modules?dir=test-extlib-module";
      inputs.logos-module-builder.follows = "logos-module-builder";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, logos-module-builder, nixpkgs, test-basic-module, test-extlib-module }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./module.yaml;
      moduleInputs = {
        test_basic_module = test-basic-module;
        test_extlib_module = test-extlib-module;
      };
    };
}
