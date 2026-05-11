{
  description = "pyCoyote Python bindings for Coyote";

  inputs = {
    self.submodules = true;

    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    { nixpkgs, flake-utils, ... }:
    let
      pycoyoteLib = {
        mkPyCoyotePackage = import ./nix/mkPyCoyotePackage.nix;
      };
      linuxSystems = builtins.filter (
        system: builtins.match ".*-linux" system != null
      ) flake-utils.lib.defaultSystems;
    in
    flake-utils.lib.eachSystem linuxSystems (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        python = pkgs.python312;
        pycoyotePackage = pycoyoteLib.mkPyCoyotePackage {
          inherit pkgs python;
          coyoteRoot = ./deps/Coyote;
          pycoyoteSrc = ./.;
        };
      in
      {
        packages = rec {
          default = pycoyote;
          pycoyote = pycoyotePackage;
        };

        checks.pycoyote-import = pkgs.runCommand "pycoyote-import" { nativeBuildInputs = [ pycoyotePackage ]; } ''
          python - <<'PY'
          import pycoyote
          print(pycoyote)
          PY
          touch $out
        '';

        devShells.default = pkgs.mkShell {
          packages = [
            pycoyotePackage
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
            pkgs.ruff
            pkgs.nixfmt-rfc-style
          ];
        };

        formatter = pkgs.nixfmt-rfc-style;
      }
    )
    // {
      lib = pycoyoteLib;
    };
}
