%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTDESDecryptCFB%+elf_symbol_type
extern n8_ippsTDESDecryptCFB%+elf_symbol_type
extern y8_ippsTDESDecryptCFB%+elf_symbol_type
extern e9_ippsTDESDecryptCFB%+elf_symbol_type
extern l9_ippsTDESDecryptCFB%+elf_symbol_type
extern n0_ippsTDESDecryptCFB%+elf_symbol_type
extern k0_ippsTDESDecryptCFB%+elf_symbol_type
extern k1_ippsTDESDecryptCFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTDESDecryptCFB
.Larraddr_ippsTDESDecryptCFB:
    dq m7_ippsTDESDecryptCFB
    dq n8_ippsTDESDecryptCFB
    dq y8_ippsTDESDecryptCFB
    dq e9_ippsTDESDecryptCFB
    dq l9_ippsTDESDecryptCFB
    dq n0_ippsTDESDecryptCFB
    dq k0_ippsTDESDecryptCFB
    dq k1_ippsTDESDecryptCFB

segment .text
global ippsTDESDecryptCFB:function (ippsTDESDecryptCFB.LEndippsTDESDecryptCFB - ippsTDESDecryptCFB)
.Lin_ippsTDESDecryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTDESDecryptCFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTDESDecryptCFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTDESDecryptCFB:
