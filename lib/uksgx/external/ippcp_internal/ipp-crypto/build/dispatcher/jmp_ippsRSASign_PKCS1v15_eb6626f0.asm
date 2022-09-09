%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSASign_PKCS1v15%+elf_symbol_type
extern n8_ippsRSASign_PKCS1v15%+elf_symbol_type
extern y8_ippsRSASign_PKCS1v15%+elf_symbol_type
extern e9_ippsRSASign_PKCS1v15%+elf_symbol_type
extern l9_ippsRSASign_PKCS1v15%+elf_symbol_type
extern n0_ippsRSASign_PKCS1v15%+elf_symbol_type
extern k0_ippsRSASign_PKCS1v15%+elf_symbol_type
extern k1_ippsRSASign_PKCS1v15%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSASign_PKCS1v15
.Larraddr_ippsRSASign_PKCS1v15:
    dq m7_ippsRSASign_PKCS1v15
    dq n8_ippsRSASign_PKCS1v15
    dq y8_ippsRSASign_PKCS1v15
    dq e9_ippsRSASign_PKCS1v15
    dq l9_ippsRSASign_PKCS1v15
    dq n0_ippsRSASign_PKCS1v15
    dq k0_ippsRSASign_PKCS1v15
    dq k1_ippsRSASign_PKCS1v15

segment .text
global ippsRSASign_PKCS1v15:function (ippsRSASign_PKCS1v15.LEndippsRSASign_PKCS1v15 - ippsRSASign_PKCS1v15)
.Lin_ippsRSASign_PKCS1v15:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSASign_PKCS1v15:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSASign_PKCS1v15]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSASign_PKCS1v15:
