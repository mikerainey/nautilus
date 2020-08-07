let pkgs = import <nixpkgs> {}; in

let mcslSrc = ../elastic-work-stealing/mcsl; in

{

  mcsl = import "${mcslSrc}/nix/default.nix" {};

  tpalSrc = ../tpal;
  
}
