#!/usr/bin/env bash

nix-build

[ "$UID" -eq 0 ] || exec sudo "$0" "$@"
OF=/dev/disk/by-id/usb-USB_2.0_USB_Flash_Drive_26B2504330140063-0\:0
sudo dd of=$OF if=./result/nautilus.iso bs=1MB && sudo sync

