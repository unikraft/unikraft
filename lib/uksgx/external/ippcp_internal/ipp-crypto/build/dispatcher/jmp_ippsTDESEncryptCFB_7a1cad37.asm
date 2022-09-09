%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTDESEncryptCFB%+elf_symbol_type
extern n8_ippsTDESEncryptCFB%+elf_symbol_type
extern y8_ippsTDESEncryptCFB%+elf_symbol_type
extern e9_ippsTDESEncryptCFB%+elf_symbol_type
extern l9_ippsTDESEncryptCFB%+elf_symbol_type
extern n0_ippsTDESEncryptCFB%+elf_symbol_type
extern k0_ippsTDESEncryptCFB%+elf_symbol_type
extern k1_ippsTDESEncryptCFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTDESEncryptCFB
.Larraddr_ippsTDESEncryptCFB:
    dq m7_ippsTDESEncryptCFB
    dq n8_ippsTDESEncryptCFB
    dq y8_ippsTDESEncryptCFB
    dq e9_ippsTDESEncryptCFB
    dq l9_ippsTDESEncryptCFB
    dq n0_ippsTDESEncryptCFB
    dq k0_ippsTDESEncryptCFB
    dq k1_ippsTDESEncryptCFB

segment .text
global ippsTDESEncryptCFB:function (ippsTDESEncryptCFB.LEndippsTDESEncryptCFB - ippsTDESEncryptCFB)
.Lin_ippsTDESEncryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTDESEncryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTDESEncryptCFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTDESEncryptCFB:
