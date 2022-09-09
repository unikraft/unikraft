%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern n8_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern y8_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern e9_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern l9_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern n0_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern k0_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern k1_ippsRSAEncrypt_OAEP%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSAEncrypt_OAEP
.Larraddr_ippsRSAEncrypt_OAEP:
    dq m7_ippsRSAEncrypt_OAEP
    dq n8_ippsRSAEncrypt_OAEP
    dq y8_ippsRSAEncrypt_OAEP
    dq e9_ippsRSAEncrypt_OAEP
    dq l9_ippsRSAEncrypt_OAEP
    dq n0_ippsRSAEncrypt_OAEP
    dq k0_ippsRSAEncrypt_OAEP
    dq k1_ippsRSAEncrypt_OAEP

segment .text
global ippsRSAEncrypt_OAEP:function (ippsRSAEncrypt_OAEP.LEndippsRSAEncrypt_OAEP - ippsRSAEncrypt_OAEP)
.Lin_ippsRSAEncrypt_OAEP:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSAEncrypt_OAEP:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSAEncrypt_OAEP]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSAEncrypt_OAEP:
