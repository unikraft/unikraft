%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSetOctString_BN%+elf_symbol_type
extern n8_ippsSetOctString_BN%+elf_symbol_type
extern y8_ippsSetOctString_BN%+elf_symbol_type
extern e9_ippsSetOctString_BN%+elf_symbol_type
extern l9_ippsSetOctString_BN%+elf_symbol_type
extern n0_ippsSetOctString_BN%+elf_symbol_type
extern k0_ippsSetOctString_BN%+elf_symbol_type
extern k1_ippsSetOctString_BN%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSetOctString_BN
.Larraddr_ippsSetOctString_BN:
    dq m7_ippsSetOctString_BN
    dq n8_ippsSetOctString_BN
    dq y8_ippsSetOctString_BN
    dq e9_ippsSetOctString_BN
    dq l9_ippsSetOctString_BN
    dq n0_ippsSetOctString_BN
    dq k0_ippsSetOctString_BN
    dq k1_ippsSetOctString_BN

segment .text
global ippsSetOctString_BN:function (ippsSetOctString_BN.LEndippsSetOctString_BN - ippsSetOctString_BN)
.Lin_ippsSetOctString_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSetOctString_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSetOctString_BN]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSetOctString_BN:
