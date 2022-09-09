%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern n8_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern y8_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern e9_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern l9_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern n0_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern k0_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern k1_ippsSMS4_CCMEncrypt%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSMS4_CCMEncrypt
.Larraddr_ippsSMS4_CCMEncrypt:
    dq m7_ippsSMS4_CCMEncrypt
    dq n8_ippsSMS4_CCMEncrypt
    dq y8_ippsSMS4_CCMEncrypt
    dq e9_ippsSMS4_CCMEncrypt
    dq l9_ippsSMS4_CCMEncrypt
    dq n0_ippsSMS4_CCMEncrypt
    dq k0_ippsSMS4_CCMEncrypt
    dq k1_ippsSMS4_CCMEncrypt

segment .text
global ippsSMS4_CCMEncrypt:function (ippsSMS4_CCMEncrypt.LEndippsSMS4_CCMEncrypt - ippsSMS4_CCMEncrypt)
.Lin_ippsSMS4_CCMEncrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSMS4_CCMEncrypt:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSMS4_CCMEncrypt]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSMS4_CCMEncrypt:
