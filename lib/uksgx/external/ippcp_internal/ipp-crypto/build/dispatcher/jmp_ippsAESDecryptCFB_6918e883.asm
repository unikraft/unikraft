%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESDecryptCFB%+elf_symbol_type
extern n8_ippsAESDecryptCFB%+elf_symbol_type
extern y8_ippsAESDecryptCFB%+elf_symbol_type
extern e9_ippsAESDecryptCFB%+elf_symbol_type
extern l9_ippsAESDecryptCFB%+elf_symbol_type
extern n0_ippsAESDecryptCFB%+elf_symbol_type
extern k0_ippsAESDecryptCFB%+elf_symbol_type
extern k1_ippsAESDecryptCFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESDecryptCFB
.Larraddr_ippsAESDecryptCFB:
    dq m7_ippsAESDecryptCFB
    dq n8_ippsAESDecryptCFB
    dq y8_ippsAESDecryptCFB
    dq e9_ippsAESDecryptCFB
    dq l9_ippsAESDecryptCFB
    dq n0_ippsAESDecryptCFB
    dq k0_ippsAESDecryptCFB
    dq k1_ippsAESDecryptCFB

segment .text
global ippsAESDecryptCFB:function (ippsAESDecryptCFB.LEndippsAESDecryptCFB - ippsAESDecryptCFB)
.Lin_ippsAESDecryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESDecryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESDecryptCFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESDecryptCFB:
