%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESEncryptCFB%+elf_symbol_type
extern n8_ippsAESEncryptCFB%+elf_symbol_type
extern y8_ippsAESEncryptCFB%+elf_symbol_type
extern e9_ippsAESEncryptCFB%+elf_symbol_type
extern l9_ippsAESEncryptCFB%+elf_symbol_type
extern n0_ippsAESEncryptCFB%+elf_symbol_type
extern k0_ippsAESEncryptCFB%+elf_symbol_type
extern k1_ippsAESEncryptCFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESEncryptCFB
.Larraddr_ippsAESEncryptCFB:
    dq m7_ippsAESEncryptCFB
    dq n8_ippsAESEncryptCFB
    dq y8_ippsAESEncryptCFB
    dq e9_ippsAESEncryptCFB
    dq l9_ippsAESEncryptCFB
    dq n0_ippsAESEncryptCFB
    dq k0_ippsAESEncryptCFB
    dq k1_ippsAESEncryptCFB

segment .text
global ippsAESEncryptCFB:function (ippsAESEncryptCFB.LEndippsAESEncryptCFB - ippsAESEncryptCFB)
.Lin_ippsAESEncryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESEncryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESEncryptCFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESEncryptCFB:
