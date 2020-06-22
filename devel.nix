{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  gcc ? pkgs.gcc,
  grub ? pkgs.grub2,
  binutils ? pkgs.binutils,
  xorriso ? pkgs.xorriso,
  nautilusSrc ? ./.,
  nautilusConfig ? "${nautilusSrc}/configs/default-config"
}:

stdenv.mkDerivation rec {
  name = "nautilus-devel";

  src = nautilusSrc;

  buildInputs = [ gcc grub xorriso binutils ];

  shellHook = ''
  echo "ln -s ${nautilusConfig} .config"
  cleanup() {
    rm -f .config
  }
  trap cleanup EXIT
  '';

}
