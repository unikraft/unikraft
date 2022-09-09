%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTDESDecryptCBC%+elf_symbol_type
extern n8_ippsTDESDecryptCBC%+elf_symbol_type
extern y8_ippsTDESDecryptCBC%+elf_symbol_type
extern e9_ippsTDESDecryptCBC%+elf_symbol_type
extern l9_ippsTDESDecryptCBC%+elf_symbol_type
extern n0_ippsTDESDecryptCBC%+elf_symbol_type
extern k0_ippsTDESDecryptCBC%+elf_symbol_type
extern k1_ippsTDESDecryptCBC%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTDESDecryptCBC
.Larraddr_ippsTDESDecryptCBC:
    dq m7_ippsTDESDecryptCBC
    dq n8_ippsTDESDecryptCBC
    dq y8_ippsTDESDecryptCBC
    dq e9_ippsTDESDecryptCBC
    dq l9_ippsTDESDecryptCBC
    dq n0_ippsTDESDecryptCBC
    dq k0_ippsTDESDecryptCBC
    dq k1_ippsTDESDecryptCBC

segment .text
global ippsTDESDecryptCBC:function (ippsTDESDecryptCBC.LEndippsTDESDecryptCBC - ippsTDESDecryptCBC)
.Lin_ippsTDESDecryptCBC:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTDESDecryptCBC:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTDESDecryptCBC]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTDESDecryptCBC:
