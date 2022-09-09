%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPUnpack%+elf_symbol_type
extern n8_ippsDLPUnpack%+elf_symbol_type
extern y8_ippsDLPUnpack%+elf_symbol_type
extern e9_ippsDLPUnpack%+elf_symbol_type
extern l9_ippsDLPUnpack%+elf_symbol_type
extern n0_ippsDLPUnpack%+elf_symbol_type
extern k0_ippsDLPUnpack%+elf_symbol_type
extern k1_ippsDLPUnpack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPUnpack
.Larraddr_ippsDLPUnpack:
    dq m7_ippsDLPUnpack
    dq n8_ippsDLPUnpack
    dq y8_ippsDLPUnpack
    dq e9_ippsDLPUnpack
    dq l9_ippsDLPUnpack
    dq n0_ippsDLPUnpack
    dq k0_ippsDLPUnpack
    dq k1_ippsDLPUnpack

segment .text
global ippsDLPUnpack:function (ippsDLPUnpack.LEndippsDLPUnpack - ippsDLPUnpack)
.Lin_ippsDLPUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPUnpack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPUnpack:
