%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSA_InitPublicKey%+elf_symbol_type
extern n8_ippsRSA_InitPublicKey%+elf_symbol_type
extern y8_ippsRSA_InitPublicKey%+elf_symbol_type
extern e9_ippsRSA_InitPublicKey%+elf_symbol_type
extern l9_ippsRSA_InitPublicKey%+elf_symbol_type
extern n0_ippsRSA_InitPublicKey%+elf_symbol_type
extern k0_ippsRSA_InitPublicKey%+elf_symbol_type
extern k1_ippsRSA_InitPublicKey%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSA_InitPublicKey
.Larraddr_ippsRSA_InitPublicKey:
    dq m7_ippsRSA_InitPublicKey
    dq n8_ippsRSA_InitPublicKey
    dq y8_ippsRSA_InitPublicKey
    dq e9_ippsRSA_InitPublicKey
    dq l9_ippsRSA_InitPublicKey
    dq n0_ippsRSA_InitPublicKey
    dq k0_ippsRSA_InitPublicKey
    dq k1_ippsRSA_InitPublicKey

segment .text
global ippsRSA_InitPublicKey:function (ippsRSA_InitPublicKey.LEndippsRSA_InitPublicKey - ippsRSA_InitPublicKey)
.Lin_ippsRSA_InitPublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSA_InitPublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSA_InitPublicKey]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSA_InitPublicKey:
