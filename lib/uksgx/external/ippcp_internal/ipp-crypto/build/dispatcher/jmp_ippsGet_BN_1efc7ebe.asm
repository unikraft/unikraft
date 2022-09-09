%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGet_BN%+elf_symbol_type
extern n8_ippsGet_BN%+elf_symbol_type
extern y8_ippsGet_BN%+elf_symbol_type
extern e9_ippsGet_BN%+elf_symbol_type
extern l9_ippsGet_BN%+elf_symbol_type
extern n0_ippsGet_BN%+elf_symbol_type
extern k0_ippsGet_BN%+elf_symbol_type
extern k1_ippsGet_BN%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGet_BN
.Larraddr_ippsGet_BN:
    dq m7_ippsGet_BN
    dq n8_ippsGet_BN
    dq y8_ippsGet_BN
    dq e9_ippsGet_BN
    dq l9_ippsGet_BN
    dq n0_ippsGet_BN
    dq k0_ippsGet_BN
    dq k1_ippsGet_BN

segment .text
global ippsGet_BN:function (ippsGet_BN.LEndippsGet_BN - ippsGet_BN)
.Lin_ippsGet_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGet_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGet_BN]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGet_BN:
