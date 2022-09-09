%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMAC_BN_I%+elf_symbol_type
extern n8_ippsMAC_BN_I%+elf_symbol_type
extern y8_ippsMAC_BN_I%+elf_symbol_type
extern e9_ippsMAC_BN_I%+elf_symbol_type
extern l9_ippsMAC_BN_I%+elf_symbol_type
extern n0_ippsMAC_BN_I%+elf_symbol_type
extern k0_ippsMAC_BN_I%+elf_symbol_type
extern k1_ippsMAC_BN_I%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMAC_BN_I
.Larraddr_ippsMAC_BN_I:
    dq m7_ippsMAC_BN_I
    dq n8_ippsMAC_BN_I
    dq y8_ippsMAC_BN_I
    dq e9_ippsMAC_BN_I
    dq l9_ippsMAC_BN_I
    dq n0_ippsMAC_BN_I
    dq k0_ippsMAC_BN_I
    dq k1_ippsMAC_BN_I

segment .text
global ippsMAC_BN_I:function (ippsMAC_BN_I.LEndippsMAC_BN_I - ippsMAC_BN_I)
.Lin_ippsMAC_BN_I:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMAC_BN_I:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMAC_BN_I]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMAC_BN_I:
