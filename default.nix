{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  gcc ? pkgs.gcc,
  grub ? pkgs.grub2,
  xorriso ? pkgs.xorriso,
  nautilusSrc ? ./.,
  nautilusConfig ? "${nautilusSrc}/configs/default-config"
}:

stdenv.mkDerivation rec {
  name = "nautilus";

  src = nautilusSrc;

  buildInputs = [ gcc grub xorriso ];

  buildPhase = ''
    cp ${nautilusConfig} .config
    make oldconfig -j
    make isoimage -j CPP_LD_LIBRARY_PATH=${stdenv.cc.cc.lib}/lib/
  '';

  installPhase = ''
    mkdir -p $out
    cp nautilus.iso $out/    
  '';

}
