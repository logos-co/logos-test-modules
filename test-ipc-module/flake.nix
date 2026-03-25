{
  description = "Test IPC Module — exercises inter-module communication via LogosAPI";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";

    test_basic_module = {
      url = "github:logos-co/logos-test-modules?dir=test-basic-module";
      inputs.logos-module-builder.follows = "logos-module-builder";
    };

    test_extlib_module = {
      url = "github:logos-co/logos-test-modules?dir=test-extlib-module";
      inputs.logos-module-builder.follows = "logos-module-builder";
    };
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;  # test_basic_module and test_extlib_module resolved from dependencies[]
    };
}
