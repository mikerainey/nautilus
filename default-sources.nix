let pkgs = import <nixpkgs> {}; in

let mcslSrc = pkgs.fetchFromGitHub {
    owner  = "mikerainey";
    repo   = "mcsl";
    rev    = "7612000608ba7a77a42d8c7490797b611aaf8c2c";
    sha256 = "0smgqdq8ykq8n8v3jdhx2m9ycgdsg36gjn3239ifk7k1wmyjwwpk";
  };
in

{

  mcsl = import "${mcslSrc}/nix/default.nix" {};

  tpalSrc = pkgs.fetchFromGitHub {
    owner  = "mikerainey";
    repo   = "tpal";
    rev    = "e27842d0994f3752b66a2eadbb0bf8fda084b05d";
    sha256 = "07bp85wa6a75bzkz66ndaaz6679r506v50nnvsacb3a6ilr4dbbb";
  };
  
}
