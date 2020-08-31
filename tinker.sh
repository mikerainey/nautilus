#!/usr/bin/env bash

nix-build --argstr nautilusConfig configs/tinker-config
# --arg sources 'import ./default-sources.nix'

scp -i ~/.ssh/id_rsa_mac result/nautilus.bin root@tinker-2.cs.iit.edu:/boot/nautilus.bin
scp -i ~/.ssh/id_rsa_mac result/nautilus.syms root@tinker-2.cs.iit.edu:/boot/nautilus.syms
