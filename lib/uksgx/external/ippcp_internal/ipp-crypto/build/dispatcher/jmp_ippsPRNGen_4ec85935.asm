%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPRNGen%+elf_symbol_type
extern n8_ippsPRNGen%+elf_symbol_type
extern y8_ippsPRNGen%+elf_symbol_type
extern e9_ippsPRNGen%+elf_symbol_type
extern l9_ippsPRNGen%+elf_symbol_type
extern n0_ippsPRNGen%+elf_symbol_type
extern k0_ippsPRNGen%+elf_symbol_type
extern k1_ippsPRNGen%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPRNGen
.Larraddr_ippsPRNGen:
    dq m7_ippsPRNGen
    dq n8_ippsPRNGen
    dq y8_ippsPRNGen
    dq e9_ippsPRNGen
    dq l9_ippsPRNGen
    dq n0_ippsPRNGen
    dq k0_ippsPRNGen
    dq k1_ippsPRNGen

segment .text
global ippsPRNGen:function (ippsPRNGen.LEndippsPRNGen - ippsPRNGen)
.Lin_ippsPRNGen:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPRNGen:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPRNGen]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPRNGen:
