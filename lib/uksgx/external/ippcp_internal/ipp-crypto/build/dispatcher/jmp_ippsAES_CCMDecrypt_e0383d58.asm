%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAES_CCMDecrypt%+elf_symbol_type
extern n8_ippsAES_CCMDecrypt%+elf_symbol_type
extern y8_ippsAES_CCMDecrypt%+elf_symbol_type
extern e9_ippsAES_CCMDecrypt%+elf_symbol_type
extern l9_ippsAES_CCMDecrypt%+elf_symbol_type
extern n0_ippsAES_CCMDecrypt%+elf_symbol_type
extern k0_ippsAES_CCMDecrypt%+elf_symbol_type
extern k1_ippsAES_CCMDecrypt%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAES_CCMDecrypt
.Larraddr_ippsAES_CCMDecrypt:
    dq m7_ippsAES_CCMDecrypt
    dq n8_ippsAES_CCMDecrypt
    dq y8_ippsAES_CCMDecrypt
    dq e9_ippsAES_CCMDecrypt
    dq l9_ippsAES_CCMDecrypt
    dq n0_ippsAES_CCMDecrypt
    dq k0_ippsAES_CCMDecrypt
    dq k1_ippsAES_CCMDecrypt

segment .text
global ippsAES_CCMDecrypt:function (ippsAES_CCMDecrypt.LEndippsAES_CCMDecrypt - ippsAES_CCMDecrypt)
.Lin_ippsAES_CCMDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAES_CCMDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAES_CCMDecrypt]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAES_CCMDecrypt:
