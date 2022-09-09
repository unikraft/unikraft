%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPPack%+elf_symbol_type
extern n8_ippsDLPPack%+elf_symbol_type
extern y8_ippsDLPPack%+elf_symbol_type
extern e9_ippsDLPPack%+elf_symbol_type
extern l9_ippsDLPPack%+elf_symbol_type
extern n0_ippsDLPPack%+elf_symbol_type
extern k0_ippsDLPPack%+elf_symbol_type
extern k1_ippsDLPPack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPPack
.Larraddr_ippsDLPPack:
    dq m7_ippsDLPPack
    dq n8_ippsDLPPack
    dq y8_ippsDLPPack
    dq e9_ippsDLPPack
    dq l9_ippsDLPPack
    dq n0_ippsDLPPack
    dq k0_ippsDLPPack
    dq k1_ippsDLPPack

segment .text
global ippsDLPPack:function (ippsDLPPack.LEndippsDLPPack - ippsDLPPack)
.Lin_ippsDLPPack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPPack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPPack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPPack:
