%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGetSize_BN%+elf_symbol_type
extern n8_ippsGetSize_BN%+elf_symbol_type
extern y8_ippsGetSize_BN%+elf_symbol_type
extern e9_ippsGetSize_BN%+elf_symbol_type
extern l9_ippsGetSize_BN%+elf_symbol_type
extern n0_ippsGetSize_BN%+elf_symbol_type
extern k0_ippsGetSize_BN%+elf_symbol_type
extern k1_ippsGetSize_BN%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGetSize_BN
.Larraddr_ippsGetSize_BN:
    dq m7_ippsGetSize_BN
    dq n8_ippsGetSize_BN
    dq y8_ippsGetSize_BN
    dq e9_ippsGetSize_BN
    dq l9_ippsGetSize_BN
    dq n0_ippsGetSize_BN
    dq k0_ippsGetSize_BN
    dq k1_ippsGetSize_BN

segment .text
global ippsGetSize_BN:function (ippsGetSize_BN.LEndippsGetSize_BN - ippsGetSize_BN)
.Lin_ippsGetSize_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGetSize_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGetSize_BN]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGetSize_BN:
