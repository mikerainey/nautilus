let pkgs = import <nixpkgs> {}; in

let mcslSrc = ../elastic-work-stealing/mcsl; in

{

  nautilusSrc = ./.;
  
  mcsl = import "${mcslSrc}/nix/default.nix" {};

  tpalSrc = ../tpal;
  
}
