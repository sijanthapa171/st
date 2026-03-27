{
  description = "Vedit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgsFor = system: import nixpkgs { inherit system; };
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in
        {
          vedit = pkgs.stdenv.mkDerivation {
            pname = "vedit";
            version = "0.1.0";
            src = ./.;

            buildInputs = with pkgs; [
              gcc
              gnumake
            ];

            buildPhase = ''
              make
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp bin/vedit $out/bin/
            '';
          };
          default = self.packages.${system}.vedit;
        }
      );

      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in
        {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              gcc
              gdb
              gnumake
              pkg-config
            ];
          };
        }
      );
    };
}
