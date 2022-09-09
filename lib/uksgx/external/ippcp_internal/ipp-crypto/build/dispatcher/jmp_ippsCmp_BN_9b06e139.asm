%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsCmp_BN%+elf_symbol_type
extern n8_ippsCmp_BN%+elf_symbol_type
extern y8_ippsCmp_BN%+elf_symbol_type
extern e9_ippsCmp_BN%+elf_symbol_type
extern l9_ippsCmp_BN%+elf_symbol_type
extern n0_ippsCmp_BN%+elf_symbol_type
extern k0_ippsCmp_BN%+elf_symbol_type
extern k1_ippsCmp_BN%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsCmp_BN
.Larraddr_ippsCmp_BN:
    dq m7_ippsCmp_BN
    dq n8_ippsCmp_BN
    dq y8_ippsCmp_BN
    dq e9_ippsCmp_BN
    dq l9_ippsCmp_BN
    dq n0_ippsCmp_BN
    dq k0_ippsCmp_BN
    dq k1_ippsCmp_BN

segment .text
global ippsCmp_BN:function (ippsCmp_BN.LEndippsCmp_BN - ippsCmp_BN)
.Lin_ippsCmp_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsCmp_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsCmp_BN]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsCmp_BN:
