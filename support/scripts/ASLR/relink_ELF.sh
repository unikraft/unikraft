#!/usr/bin/sh

if [ "$#" -ne 3 ]; then
    echo " Script usage : relink_ELF BUILD_DIR NAME_EXEC UNIKRAFT_FOLDER"
else
    libkvm=$1/libkvmplat
    lib=$(python3.10 string_script.py "$(make print-libs)")
    addr=$(python3.10 $3/support/scripts/ASLR/ASLR.py --setup_file="$3/plat/kvm/x86/multiboot.S" --base_addr="-2")
    #Shuffles the linking script
    $3/support/scripts/ASLR/ASLR.py --file_path=$libkvm/link64.lds --lib_list="$lib" --output_path=$libkvm/link64_ASLR.lds --base_addr="$addr"
    gcc -nostdlib -Wl,--omagic -Wl,--build-id=none -nostdinc -no-pie  -Wl,-m,elf_x86_64 -T$libkvm/link64_ASLR.lds -L$1 -o $1/$2.dbg
    python3 $3/support/scripts/sect-strip.py --with-objcopy=""objcopy $1/$2.dbg -o $1/$2 && ""strip -s $1/$2
fi
