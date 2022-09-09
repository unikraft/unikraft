%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern n8_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern y8_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern e9_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern l9_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern n0_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern k0_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern k1_ippsRSADecrypt_PKCSv15%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSADecrypt_PKCSv15
.Larraddr_ippsRSADecrypt_PKCSv15:
    dq m7_ippsRSADecrypt_PKCSv15
    dq n8_ippsRSADecrypt_PKCSv15
    dq y8_ippsRSADecrypt_PKCSv15
    dq e9_ippsRSADecrypt_PKCSv15
    dq l9_ippsRSADecrypt_PKCSv15
    dq n0_ippsRSADecrypt_PKCSv15
    dq k0_ippsRSADecrypt_PKCSv15
    dq k1_ippsRSADecrypt_PKCSv15

segment .text
global ippsRSADecrypt_PKCSv15:function (ippsRSADecrypt_PKCSv15.LEndippsRSADecrypt_PKCSv15 - ippsRSADecrypt_PKCSv15)
.Lin_ippsRSADecrypt_PKCSv15:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSADecrypt_PKCSv15:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSADecrypt_PKCSv15]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSADecrypt_PKCSv15:
