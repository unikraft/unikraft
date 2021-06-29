#!/bin/bash

if [ ! -d "build/isofiles/" ]; then
    mkdir ./build/isofiles
    mkdir ./build/isofiles/boot
    mkdir ./build/isofiles/boot/grub
    printf "set default=0\nset timeout=0\n\nmenuentry \"Unikraft\" \
    {\n\tmultiboot2 /boot/unikraft_kvm-x86_64\n\tboot\n}" \
    > ./build/isofiles/boot/grub/grub.cfg
fi

cp ./build/unikraft_kvm-x86_64 build/isofiles/boot/ && \
grub-mkrescue /usr/lib/grub/i386-pc -o \
./build/unikraft_kvm-x86_64.iso ./build/isofiles/ && \
sudo qemu-system-x86_64 -enable-kvm -m 128 \
-cpu host -cdrom ./build/unikraft_kvm-x86_64.iso -serial stdio
