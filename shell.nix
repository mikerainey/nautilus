# Usage:
#   $ nix-shell
#
# To run nautilus via default.nix:
#   $ nautilus
#   ... boots nautilus in qemu ...
#
# To run nautilus locally:
#   $ nautilus_link_tpal
#   ... sets up sym links, assuming tpal and mcsl are located in ../ ...
#  $ nautilus-local
#  ... boots nautilus in qemu ...

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  gcc ? pkgs.gcc,
  grub ? pkgs.grub2,
  binutils ? pkgs.binutils,
  xorriso ? pkgs.xorriso,
  sources ? import ./local-sources.nix,
  mcsl ? sources.mcsl,
  tpalSrc ? sources.tpalSrc,
  nautilusSrc ? sources.nautilusSrc,
  nautilusConfig ? "${nautilusSrc}/configs/default-config",
  nautilus ? import ./default.nix { pkgs = pkgs;
                                    sources = sources;
                                    nautilusConfig = nautilusConfig;
                                    grub = grub;
                                    binutils = binutils;
                                    xorriso = xorriso;
                                  },
  qemu ? pkgs.qemu
}:
  
stdenv.mkDerivation {
  name = "nautilus-shell";
  buildInputs = [ qemu nautilus gcc grub xorriso binutils ];
  shellHook = ''
    alias nautilus="qemu-system-x86_64 -cdrom ${nautilus}/nautilus.iso -m 16000 -curses -nographic -smp 4"
    alias nautilus_link_tpal="./scripts/tpal.sh `pwd`/../tpal `pwd`/../mcsl `pwd`"
    alias nautilus_local="qemu-system-x86_64 -cdrom ./nautilus.iso -m 16000 -curses -nographic -smp 4"
  '';
}
