%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMul_BN%+elf_symbol_type
extern n8_ippsMul_BN%+elf_symbol_type
extern y8_ippsMul_BN%+elf_symbol_type
extern e9_ippsMul_BN%+elf_symbol_type
extern l9_ippsMul_BN%+elf_symbol_type
extern n0_ippsMul_BN%+elf_symbol_type
extern k0_ippsMul_BN%+elf_symbol_type
extern k1_ippsMul_BN%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMul_BN
.Larraddr_ippsMul_BN:
    dq m7_ippsMul_BN
    dq n8_ippsMul_BN
    dq y8_ippsMul_BN
    dq e9_ippsMul_BN
    dq l9_ippsMul_BN
    dq n0_ippsMul_BN
    dq k0_ippsMul_BN
    dq k1_ippsMul_BN

segment .text
global ippsMul_BN:function (ippsMul_BN.LEndippsMul_BN - ippsMul_BN)
.Lin_ippsMul_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMul_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMul_BN]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMul_BN:
