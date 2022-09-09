%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsARCFourDecrypt%+elf_symbol_type
extern n8_ippsARCFourDecrypt%+elf_symbol_type
extern y8_ippsARCFourDecrypt%+elf_symbol_type
extern e9_ippsARCFourDecrypt%+elf_symbol_type
extern l9_ippsARCFourDecrypt%+elf_symbol_type
extern n0_ippsARCFourDecrypt%+elf_symbol_type
extern k0_ippsARCFourDecrypt%+elf_symbol_type
extern k1_ippsARCFourDecrypt%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsARCFourDecrypt
.Larraddr_ippsARCFourDecrypt:
    dq m7_ippsARCFourDecrypt
    dq n8_ippsARCFourDecrypt
    dq y8_ippsARCFourDecrypt
    dq e9_ippsARCFourDecrypt
    dq l9_ippsARCFourDecrypt
    dq n0_ippsARCFourDecrypt
    dq k0_ippsARCFourDecrypt
    dq k1_ippsARCFourDecrypt

segment .text
global ippsARCFourDecrypt:function (ippsARCFourDecrypt.LEndippsARCFourDecrypt - ippsARCFourDecrypt)
.Lin_ippsARCFourDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsARCFourDecrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsARCFourDecrypt]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsARCFourDecrypt:
