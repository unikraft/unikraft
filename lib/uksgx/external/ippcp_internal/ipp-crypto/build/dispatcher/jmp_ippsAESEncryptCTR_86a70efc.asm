%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESEncryptCTR%+elf_symbol_type
extern n8_ippsAESEncryptCTR%+elf_symbol_type
extern y8_ippsAESEncryptCTR%+elf_symbol_type
extern e9_ippsAESEncryptCTR%+elf_symbol_type
extern l9_ippsAESEncryptCTR%+elf_symbol_type
extern n0_ippsAESEncryptCTR%+elf_symbol_type
extern k0_ippsAESEncryptCTR%+elf_symbol_type
extern k1_ippsAESEncryptCTR%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESEncryptCTR
.Larraddr_ippsAESEncryptCTR:
    dq m7_ippsAESEncryptCTR
    dq n8_ippsAESEncryptCTR
    dq y8_ippsAESEncryptCTR
    dq e9_ippsAESEncryptCTR
    dq l9_ippsAESEncryptCTR
    dq n0_ippsAESEncryptCTR
    dq k0_ippsAESEncryptCTR
    dq k1_ippsAESEncryptCTR

segment .text
global ippsAESEncryptCTR:function (ippsAESEncryptCTR.LEndippsAESEncryptCTR - ippsAESEncryptCTR)
.Lin_ippsAESEncryptCTR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESEncryptCTR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESEncryptCTR]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESEncryptCTR:
