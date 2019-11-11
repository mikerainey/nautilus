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
  name = "nautilus";

  src = nautilusSrc;

  buildInputs = [ gcc grub xorriso binutils ];

  buildPhase = ''
    cp ${nautilusConfig} .config
    make oldconfig -j
    make isoimage -j KBUILD_VERBOSE=1
  '';

  installPhase = ''
    mkdir -p $out
    cp nautilus.iso $out/    
  '';

}
