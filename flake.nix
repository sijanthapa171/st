{
  description = "Terminal notepad in C (flake dev shell + package)";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0";

  outputs =
    { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSupportedSystem =
        f: nixpkgs.lib.genAttrs supportedSystems (system: f (import nixpkgs { inherit system; }));
    in
    {
      packages = forEachSupportedSystem (pkgs: {
        default = pkgs.stdenv.mkDerivation {
          pname = "notepad";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [ gnumake ];

          buildPhase = ''
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp notepad $out/bin/notepad
          '';
        };
      });

      apps = forEachSupportedSystem (pkgs: {
        default = {
          type = "app";
          program = "${self.packages.${pkgs.system}.default}/bin/notepad";
        };
      });

      devShells = forEachSupportedSystem (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            gcc
            gnumake
            clang-tools
            cppcheck
            valgrind
          ] ++ (if pkgs.stdenv.isDarwin then [ ] else [ gdb ]);
        };
      });
    };
}
