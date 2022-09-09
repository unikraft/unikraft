%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESEncryptECB%+elf_symbol_type
extern n8_ippsAESEncryptECB%+elf_symbol_type
extern y8_ippsAESEncryptECB%+elf_symbol_type
extern e9_ippsAESEncryptECB%+elf_symbol_type
extern l9_ippsAESEncryptECB%+elf_symbol_type
extern n0_ippsAESEncryptECB%+elf_symbol_type
extern k0_ippsAESEncryptECB%+elf_symbol_type
extern k1_ippsAESEncryptECB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESEncryptECB
.Larraddr_ippsAESEncryptECB:
    dq m7_ippsAESEncryptECB
    dq n8_ippsAESEncryptECB
    dq y8_ippsAESEncryptECB
    dq e9_ippsAESEncryptECB
    dq l9_ippsAESEncryptECB
    dq n0_ippsAESEncryptECB
    dq k0_ippsAESEncryptECB
    dq k1_ippsAESEncryptECB

segment .text
global ippsAESEncryptECB:function (ippsAESEncryptECB.LEndippsAESEncryptECB - ippsAESEncryptECB)
.Lin_ippsAESEncryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESEncryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESEncryptECB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESEncryptECB:
