%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPRNGInit%+elf_symbol_type
extern n8_ippsPRNGInit%+elf_symbol_type
extern y8_ippsPRNGInit%+elf_symbol_type
extern e9_ippsPRNGInit%+elf_symbol_type
extern l9_ippsPRNGInit%+elf_symbol_type
extern n0_ippsPRNGInit%+elf_symbol_type
extern k0_ippsPRNGInit%+elf_symbol_type
extern k1_ippsPRNGInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPRNGInit
.Larraddr_ippsPRNGInit:
    dq m7_ippsPRNGInit
    dq n8_ippsPRNGInit
    dq y8_ippsPRNGInit
    dq e9_ippsPRNGInit
    dq l9_ippsPRNGInit
    dq n0_ippsPRNGInit
    dq k0_ippsPRNGInit
    dq k1_ippsPRNGInit

segment .text
global ippsPRNGInit:function (ippsPRNGInit.LEndippsPRNGInit - ippsPRNGInit)
.Lin_ippsPRNGInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPRNGInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPRNGInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPRNGInit:
