let pkgs = import <nixpkgs> {}; in

let mcslSrc = ../mcsl; in

{

  nautilusSrc = ./.;
  
  mcsl = import "${mcslSrc}/nix/default.nix" {};

  tpalSrc = ../tpal;
  
}
