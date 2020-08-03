{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  gcc ? pkgs.gcc,
  grub ? pkgs.grub2,
  binutils ? pkgs.binutils,
  xorriso ? pkgs.xorriso,
  mcslSrc ? ../elastic-work-stealing/mcsl,
  mcsl ? import "${mcslSrc}/nix/default.nix" {},
  tpalSrc ? ../tpal,
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
    cp --remove-destination ${mcsl}/include/*.hpp include/rt/mcsl/
    cp --remove-destination ${tpalSrc}/runtime/include/*.hpp include/rt/tpal
    cp --remove-destination ${tpalSrc}/runtime/bench/*.hpp src/rt/tpal
    cp --remove-destination ${tpalSrc}/runtime/bench/*_manual.s src/rt/tpal
    make isoimage -j KBUILD_VERBOSE=1
  '';

  installPhase = ''
    mkdir -p $out
    cp nautilus.iso nautilus.bin nautilus.syms $out/    
  '';

}
