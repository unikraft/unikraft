%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTDESEncryptOFB%+elf_symbol_type
extern n8_ippsTDESEncryptOFB%+elf_symbol_type
extern y8_ippsTDESEncryptOFB%+elf_symbol_type
extern e9_ippsTDESEncryptOFB%+elf_symbol_type
extern l9_ippsTDESEncryptOFB%+elf_symbol_type
extern n0_ippsTDESEncryptOFB%+elf_symbol_type
extern k0_ippsTDESEncryptOFB%+elf_symbol_type
extern k1_ippsTDESEncryptOFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTDESEncryptOFB
.Larraddr_ippsTDESEncryptOFB:
    dq m7_ippsTDESEncryptOFB
    dq n8_ippsTDESEncryptOFB
    dq y8_ippsTDESEncryptOFB
    dq e9_ippsTDESEncryptOFB
    dq l9_ippsTDESEncryptOFB
    dq n0_ippsTDESEncryptOFB
    dq k0_ippsTDESEncryptOFB
    dq k1_ippsTDESEncryptOFB

segment .text
global ippsTDESEncryptOFB:function (ippsTDESEncryptOFB.LEndippsTDESEncryptOFB - ippsTDESEncryptOFB)
.Lin_ippsTDESEncryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTDESEncryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTDESEncryptOFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTDESEncryptOFB:
