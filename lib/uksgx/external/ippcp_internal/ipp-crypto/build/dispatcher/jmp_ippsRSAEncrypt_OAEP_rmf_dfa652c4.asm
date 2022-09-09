%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern n8_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern y8_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern e9_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern l9_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern n0_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern k0_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern k1_ippsRSAEncrypt_OAEP_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSAEncrypt_OAEP_rmf
.Larraddr_ippsRSAEncrypt_OAEP_rmf:
    dq m7_ippsRSAEncrypt_OAEP_rmf
    dq n8_ippsRSAEncrypt_OAEP_rmf
    dq y8_ippsRSAEncrypt_OAEP_rmf
    dq e9_ippsRSAEncrypt_OAEP_rmf
    dq l9_ippsRSAEncrypt_OAEP_rmf
    dq n0_ippsRSAEncrypt_OAEP_rmf
    dq k0_ippsRSAEncrypt_OAEP_rmf
    dq k1_ippsRSAEncrypt_OAEP_rmf

segment .text
global ippsRSAEncrypt_OAEP_rmf:function (ippsRSAEncrypt_OAEP_rmf.LEndippsRSAEncrypt_OAEP_rmf - ippsRSAEncrypt_OAEP_rmf)
.Lin_ippsRSAEncrypt_OAEP_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSAEncrypt_OAEP_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSAEncrypt_OAEP_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSAEncrypt_OAEP_rmf:
