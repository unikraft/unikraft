%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern n8_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern y8_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern e9_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern l9_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern n0_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern k0_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern k1_ippsRSA_InitPrivateKeyType2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSA_InitPrivateKeyType2
.Larraddr_ippsRSA_InitPrivateKeyType2:
    dq m7_ippsRSA_InitPrivateKeyType2
    dq n8_ippsRSA_InitPrivateKeyType2
    dq y8_ippsRSA_InitPrivateKeyType2
    dq e9_ippsRSA_InitPrivateKeyType2
    dq l9_ippsRSA_InitPrivateKeyType2
    dq n0_ippsRSA_InitPrivateKeyType2
    dq k0_ippsRSA_InitPrivateKeyType2
    dq k1_ippsRSA_InitPrivateKeyType2

segment .text
global ippsRSA_InitPrivateKeyType2:function (ippsRSA_InitPrivateKeyType2.LEndippsRSA_InitPrivateKeyType2 - ippsRSA_InitPrivateKeyType2)
.Lin_ippsRSA_InitPrivateKeyType2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSA_InitPrivateKeyType2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSA_InitPrivateKeyType2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSA_InitPrivateKeyType2:
