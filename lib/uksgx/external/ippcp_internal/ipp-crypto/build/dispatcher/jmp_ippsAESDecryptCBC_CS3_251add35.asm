%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern n8_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern y8_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern e9_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern l9_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern n0_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern k0_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern k1_ippsAESDecryptCBC_CS3%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESDecryptCBC_CS3
.Larraddr_ippsAESDecryptCBC_CS3:
    dq m7_ippsAESDecryptCBC_CS3
    dq n8_ippsAESDecryptCBC_CS3
    dq y8_ippsAESDecryptCBC_CS3
    dq e9_ippsAESDecryptCBC_CS3
    dq l9_ippsAESDecryptCBC_CS3
    dq n0_ippsAESDecryptCBC_CS3
    dq k0_ippsAESDecryptCBC_CS3
    dq k1_ippsAESDecryptCBC_CS3

segment .text
global ippsAESDecryptCBC_CS3:function (ippsAESDecryptCBC_CS3.LEndippsAESDecryptCBC_CS3 - ippsAESDecryptCBC_CS3)
.Lin_ippsAESDecryptCBC_CS3:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESDecryptCBC_CS3:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESDecryptCBC_CS3]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESDecryptCBC_CS3:
