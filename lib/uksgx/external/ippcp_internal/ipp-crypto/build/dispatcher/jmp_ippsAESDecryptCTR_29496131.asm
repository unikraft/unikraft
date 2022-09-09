%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESDecryptCTR%+elf_symbol_type
extern n8_ippsAESDecryptCTR%+elf_symbol_type
extern y8_ippsAESDecryptCTR%+elf_symbol_type
extern e9_ippsAESDecryptCTR%+elf_symbol_type
extern l9_ippsAESDecryptCTR%+elf_symbol_type
extern n0_ippsAESDecryptCTR%+elf_symbol_type
extern k0_ippsAESDecryptCTR%+elf_symbol_type
extern k1_ippsAESDecryptCTR%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESDecryptCTR
.Larraddr_ippsAESDecryptCTR:
    dq m7_ippsAESDecryptCTR
    dq n8_ippsAESDecryptCTR
    dq y8_ippsAESDecryptCTR
    dq e9_ippsAESDecryptCTR
    dq l9_ippsAESDecryptCTR
    dq n0_ippsAESDecryptCTR
    dq k0_ippsAESDecryptCTR
    dq k1_ippsAESDecryptCTR

segment .text
global ippsAESDecryptCTR:function (ippsAESDecryptCTR.LEndippsAESDecryptCTR - ippsAESDecryptCTR)
.Lin_ippsAESDecryptCTR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESDecryptCTR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESDecryptCTR]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESDecryptCTR:
