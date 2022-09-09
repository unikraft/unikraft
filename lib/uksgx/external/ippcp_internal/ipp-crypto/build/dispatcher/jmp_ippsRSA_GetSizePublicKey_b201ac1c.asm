%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern n8_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern y8_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern e9_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern l9_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern n0_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern k0_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern k1_ippsRSA_GetSizePublicKey%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSA_GetSizePublicKey
.Larraddr_ippsRSA_GetSizePublicKey:
    dq m7_ippsRSA_GetSizePublicKey
    dq n8_ippsRSA_GetSizePublicKey
    dq y8_ippsRSA_GetSizePublicKey
    dq e9_ippsRSA_GetSizePublicKey
    dq l9_ippsRSA_GetSizePublicKey
    dq n0_ippsRSA_GetSizePublicKey
    dq k0_ippsRSA_GetSizePublicKey
    dq k1_ippsRSA_GetSizePublicKey

segment .text
global ippsRSA_GetSizePublicKey:function (ippsRSA_GetSizePublicKey.LEndippsRSA_GetSizePublicKey - ippsRSA_GetSizePublicKey)
.Lin_ippsRSA_GetSizePublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSA_GetSizePublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSA_GetSizePublicKey]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSA_GetSizePublicKey:
