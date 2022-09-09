%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESEncryptOFB%+elf_symbol_type
extern n8_ippsAESEncryptOFB%+elf_symbol_type
extern y8_ippsAESEncryptOFB%+elf_symbol_type
extern e9_ippsAESEncryptOFB%+elf_symbol_type
extern l9_ippsAESEncryptOFB%+elf_symbol_type
extern n0_ippsAESEncryptOFB%+elf_symbol_type
extern k0_ippsAESEncryptOFB%+elf_symbol_type
extern k1_ippsAESEncryptOFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESEncryptOFB
.Larraddr_ippsAESEncryptOFB:
    dq m7_ippsAESEncryptOFB
    dq n8_ippsAESEncryptOFB
    dq y8_ippsAESEncryptOFB
    dq e9_ippsAESEncryptOFB
    dq l9_ippsAESEncryptOFB
    dq n0_ippsAESEncryptOFB
    dq k0_ippsAESEncryptOFB
    dq k1_ippsAESEncryptOFB

segment .text
global ippsAESEncryptOFB:function (ippsAESEncryptOFB.LEndippsAESEncryptOFB - ippsAESEncryptOFB)
.Lin_ippsAESEncryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESEncryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESEncryptOFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESEncryptOFB:
