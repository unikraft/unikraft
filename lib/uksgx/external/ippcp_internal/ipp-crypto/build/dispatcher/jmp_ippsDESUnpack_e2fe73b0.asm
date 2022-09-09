%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDESUnpack%+elf_symbol_type
extern n8_ippsDESUnpack%+elf_symbol_type
extern y8_ippsDESUnpack%+elf_symbol_type
extern e9_ippsDESUnpack%+elf_symbol_type
extern l9_ippsDESUnpack%+elf_symbol_type
extern n0_ippsDESUnpack%+elf_symbol_type
extern k0_ippsDESUnpack%+elf_symbol_type
extern k1_ippsDESUnpack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDESUnpack
.Larraddr_ippsDESUnpack:
    dq m7_ippsDESUnpack
    dq n8_ippsDESUnpack
    dq y8_ippsDESUnpack
    dq e9_ippsDESUnpack
    dq l9_ippsDESUnpack
    dq n0_ippsDESUnpack
    dq k0_ippsDESUnpack
    dq k1_ippsDESUnpack

segment .text
global ippsDESUnpack:function (ippsDESUnpack.LEndippsDESUnpack - ippsDESUnpack)
.Lin_ippsDESUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDESUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDESUnpack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDESUnpack:
