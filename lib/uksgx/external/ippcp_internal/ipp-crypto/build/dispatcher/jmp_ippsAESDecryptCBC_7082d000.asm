%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESDecryptCBC%+elf_symbol_type
extern n8_ippsAESDecryptCBC%+elf_symbol_type
extern y8_ippsAESDecryptCBC%+elf_symbol_type
extern e9_ippsAESDecryptCBC%+elf_symbol_type
extern l9_ippsAESDecryptCBC%+elf_symbol_type
extern n0_ippsAESDecryptCBC%+elf_symbol_type
extern k0_ippsAESDecryptCBC%+elf_symbol_type
extern k1_ippsAESDecryptCBC%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESDecryptCBC
.Larraddr_ippsAESDecryptCBC:
    dq m7_ippsAESDecryptCBC
    dq n8_ippsAESDecryptCBC
    dq y8_ippsAESDecryptCBC
    dq e9_ippsAESDecryptCBC
    dq l9_ippsAESDecryptCBC
    dq n0_ippsAESDecryptCBC
    dq k0_ippsAESDecryptCBC
    dq k1_ippsAESDecryptCBC

segment .text
global ippsAESDecryptCBC:function (ippsAESDecryptCBC.LEndippsAESDecryptCBC - ippsAESDecryptCBC)
.Lin_ippsAESDecryptCBC:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESDecryptCBC:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESDecryptCBC]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESDecryptCBC:
