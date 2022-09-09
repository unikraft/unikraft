%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESDecryptOFB%+elf_symbol_type
extern n8_ippsAESDecryptOFB%+elf_symbol_type
extern y8_ippsAESDecryptOFB%+elf_symbol_type
extern e9_ippsAESDecryptOFB%+elf_symbol_type
extern l9_ippsAESDecryptOFB%+elf_symbol_type
extern n0_ippsAESDecryptOFB%+elf_symbol_type
extern k0_ippsAESDecryptOFB%+elf_symbol_type
extern k1_ippsAESDecryptOFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESDecryptOFB
.Larraddr_ippsAESDecryptOFB:
    dq m7_ippsAESDecryptOFB
    dq n8_ippsAESDecryptOFB
    dq y8_ippsAESDecryptOFB
    dq e9_ippsAESDecryptOFB
    dq l9_ippsAESDecryptOFB
    dq n0_ippsAESDecryptOFB
    dq k0_ippsAESDecryptOFB
    dq k1_ippsAESDecryptOFB

segment .text
global ippsAESDecryptOFB:function (ippsAESDecryptOFB.LEndippsAESDecryptOFB - ippsAESDecryptOFB)
.Lin_ippsAESDecryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESDecryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESDecryptOFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESDecryptOFB:
