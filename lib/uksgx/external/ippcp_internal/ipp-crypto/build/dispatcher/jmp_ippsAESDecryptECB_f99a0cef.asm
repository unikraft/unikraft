%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESDecryptECB%+elf_symbol_type
extern n8_ippsAESDecryptECB%+elf_symbol_type
extern y8_ippsAESDecryptECB%+elf_symbol_type
extern e9_ippsAESDecryptECB%+elf_symbol_type
extern l9_ippsAESDecryptECB%+elf_symbol_type
extern n0_ippsAESDecryptECB%+elf_symbol_type
extern k0_ippsAESDecryptECB%+elf_symbol_type
extern k1_ippsAESDecryptECB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESDecryptECB
.Larraddr_ippsAESDecryptECB:
    dq m7_ippsAESDecryptECB
    dq n8_ippsAESDecryptECB
    dq y8_ippsAESDecryptECB
    dq e9_ippsAESDecryptECB
    dq l9_ippsAESDecryptECB
    dq n0_ippsAESDecryptECB
    dq k0_ippsAESDecryptECB
    dq k1_ippsAESDecryptECB

segment .text
global ippsAESDecryptECB:function (ippsAESDecryptECB.LEndippsAESDecryptECB - ippsAESDecryptECB)
.Lin_ippsAESDecryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESDecryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESDecryptECB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESDecryptECB:
