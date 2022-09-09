%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAES_GCMDecrypt%+elf_symbol_type
extern n8_ippsAES_GCMDecrypt%+elf_symbol_type
extern y8_ippsAES_GCMDecrypt%+elf_symbol_type
extern e9_ippsAES_GCMDecrypt%+elf_symbol_type
extern l9_ippsAES_GCMDecrypt%+elf_symbol_type
extern n0_ippsAES_GCMDecrypt%+elf_symbol_type
extern k0_ippsAES_GCMDecrypt%+elf_symbol_type
extern k1_ippsAES_GCMDecrypt%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAES_GCMDecrypt
.Larraddr_ippsAES_GCMDecrypt:
    dq m7_ippsAES_GCMDecrypt
    dq n8_ippsAES_GCMDecrypt
    dq y8_ippsAES_GCMDecrypt
    dq e9_ippsAES_GCMDecrypt
    dq l9_ippsAES_GCMDecrypt
    dq n0_ippsAES_GCMDecrypt
    dq k0_ippsAES_GCMDecrypt
    dq k1_ippsAES_GCMDecrypt

segment .text
global ippsAES_GCMDecrypt:function (ippsAES_GCMDecrypt.LEndippsAES_GCMDecrypt - ippsAES_GCMDecrypt)
.Lin_ippsAES_GCMDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAES_GCMDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAES_GCMDecrypt]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAES_GCMDecrypt:
