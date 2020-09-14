# Usage:
#   $ nix-shell
#   ... nautilus build ...
#   $ nautilus
#   ... boot nautilus in qemu ...

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  nautilusSrc ? ./.,
  nautilusConfig ? "${nautilusSrc}/configs/default-config",
  nautilus ? import ./default.nix { pkgs = pkgs;
                                    nautilusSrc = nautilusSrc;
                                    nautilusConfig = nautilusConfig;
                                  },
  qemu ? pkgs.qemu
}:
  
stdenv.mkDerivation {
  name = "nautilus-shell";
  buildInputs = [ qemu nautilus ];
  shellHook = ''
    alias nautilus="qemu-system-x86_64 -cdrom ${nautilus}/nautilus.iso -m 2048 -curses -nographic -smp 4"
  '';
}
