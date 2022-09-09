%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsARCFourEncrypt%+elf_symbol_type
extern n8_ippsARCFourEncrypt%+elf_symbol_type
extern y8_ippsARCFourEncrypt%+elf_symbol_type
extern e9_ippsARCFourEncrypt%+elf_symbol_type
extern l9_ippsARCFourEncrypt%+elf_symbol_type
extern n0_ippsARCFourEncrypt%+elf_symbol_type
extern k0_ippsARCFourEncrypt%+elf_symbol_type
extern k1_ippsARCFourEncrypt%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsARCFourEncrypt
.Larraddr_ippsARCFourEncrypt:
    dq m7_ippsARCFourEncrypt
    dq n8_ippsARCFourEncrypt
    dq y8_ippsARCFourEncrypt
    dq e9_ippsARCFourEncrypt
    dq l9_ippsARCFourEncrypt
    dq n0_ippsARCFourEncrypt
    dq k0_ippsARCFourEncrypt
    dq k1_ippsARCFourEncrypt

segment .text
global ippsARCFourEncrypt:function (ippsARCFourEncrypt.LEndippsARCFourEncrypt - ippsARCFourEncrypt)
.Lin_ippsARCFourEncrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsARCFourEncrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsARCFourEncrypt]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsARCFourEncrypt:
