%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern n8_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern y8_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern e9_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern l9_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern n0_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern k0_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern k1_ippsRSA_GetSizePrivateKeyType2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSA_GetSizePrivateKeyType2
.Larraddr_ippsRSA_GetSizePrivateKeyType2:
    dq m7_ippsRSA_GetSizePrivateKeyType2
    dq n8_ippsRSA_GetSizePrivateKeyType2
    dq y8_ippsRSA_GetSizePrivateKeyType2
    dq e9_ippsRSA_GetSizePrivateKeyType2
    dq l9_ippsRSA_GetSizePrivateKeyType2
    dq n0_ippsRSA_GetSizePrivateKeyType2
    dq k0_ippsRSA_GetSizePrivateKeyType2
    dq k1_ippsRSA_GetSizePrivateKeyType2

segment .text
global ippsRSA_GetSizePrivateKeyType2:function (ippsRSA_GetSizePrivateKeyType2.LEndippsRSA_GetSizePrivateKeyType2 - ippsRSA_GetSizePrivateKeyType2)
.Lin_ippsRSA_GetSizePrivateKeyType2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSA_GetSizePrivateKeyType2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSA_GetSizePrivateKeyType2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSA_GetSizePrivateKeyType2:
