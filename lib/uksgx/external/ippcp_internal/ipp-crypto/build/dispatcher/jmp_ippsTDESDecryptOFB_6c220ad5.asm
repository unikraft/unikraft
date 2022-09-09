%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTDESDecryptOFB%+elf_symbol_type
extern n8_ippsTDESDecryptOFB%+elf_symbol_type
extern y8_ippsTDESDecryptOFB%+elf_symbol_type
extern e9_ippsTDESDecryptOFB%+elf_symbol_type
extern l9_ippsTDESDecryptOFB%+elf_symbol_type
extern n0_ippsTDESDecryptOFB%+elf_symbol_type
extern k0_ippsTDESDecryptOFB%+elf_symbol_type
extern k1_ippsTDESDecryptOFB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTDESDecryptOFB
.Larraddr_ippsTDESDecryptOFB:
    dq m7_ippsTDESDecryptOFB
    dq n8_ippsTDESDecryptOFB
    dq y8_ippsTDESDecryptOFB
    dq e9_ippsTDESDecryptOFB
    dq l9_ippsTDESDecryptOFB
    dq n0_ippsTDESDecryptOFB
    dq k0_ippsTDESDecryptOFB
    dq k1_ippsTDESDecryptOFB

segment .text
global ippsTDESDecryptOFB:function (ippsTDESDecryptOFB.LEndippsTDESDecryptOFB - ippsTDESDecryptOFB)
.Lin_ippsTDESDecryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTDESDecryptOFB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTDESDecryptOFB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTDESDecryptOFB:
