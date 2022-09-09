%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAES_XTSDecrypt%+elf_symbol_type
extern n8_ippsAES_XTSDecrypt%+elf_symbol_type
extern y8_ippsAES_XTSDecrypt%+elf_symbol_type
extern e9_ippsAES_XTSDecrypt%+elf_symbol_type
extern l9_ippsAES_XTSDecrypt%+elf_symbol_type
extern n0_ippsAES_XTSDecrypt%+elf_symbol_type
extern k0_ippsAES_XTSDecrypt%+elf_symbol_type
extern k1_ippsAES_XTSDecrypt%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAES_XTSDecrypt
.Larraddr_ippsAES_XTSDecrypt:
    dq m7_ippsAES_XTSDecrypt
    dq n8_ippsAES_XTSDecrypt
    dq y8_ippsAES_XTSDecrypt
    dq e9_ippsAES_XTSDecrypt
    dq l9_ippsAES_XTSDecrypt
    dq n0_ippsAES_XTSDecrypt
    dq k0_ippsAES_XTSDecrypt
    dq k1_ippsAES_XTSDecrypt

segment .text
global ippsAES_XTSDecrypt:function (ippsAES_XTSDecrypt.LEndippsAES_XTSDecrypt - ippsAES_XTSDecrypt)
.Lin_ippsAES_XTSDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAES_XTSDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAES_XTSDecrypt]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAES_XTSDecrypt:
