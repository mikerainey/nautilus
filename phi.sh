#!/usr/bin/env bash

nix-build --argstr nautilusConfig configs/tinker-config
# --arg sources 'import ./default-sources.nix'

scp -i ~/.ssh/id_rsa_mac result/nautilus.bin root@v-test-5038ki.cs.northwestern.edu:/boot/nautilus-rainey.bin
scp -i ~/.ssh/id_rsa_mac result/nautilus.syms root@v-test-5038ki.cs.northwestern.edu:/boot/nautilus-rainey.syms
