%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDiv_BN%+elf_symbol_type
extern n8_ippsDiv_BN%+elf_symbol_type
extern y8_ippsDiv_BN%+elf_symbol_type
extern e9_ippsDiv_BN%+elf_symbol_type
extern l9_ippsDiv_BN%+elf_symbol_type
extern n0_ippsDiv_BN%+elf_symbol_type
extern k0_ippsDiv_BN%+elf_symbol_type
extern k1_ippsDiv_BN%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDiv_BN
.Larraddr_ippsDiv_BN:
    dq m7_ippsDiv_BN
    dq n8_ippsDiv_BN
    dq y8_ippsDiv_BN
    dq e9_ippsDiv_BN
    dq l9_ippsDiv_BN
    dq n0_ippsDiv_BN
    dq k0_ippsDiv_BN
    dq k1_ippsDiv_BN

segment .text
global ippsDiv_BN:function (ippsDiv_BN.LEndippsDiv_BN - ippsDiv_BN)
.Lin_ippsDiv_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDiv_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDiv_BN]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDiv_BN:
