%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPRNGSetSeed%+elf_symbol_type
extern n8_ippsPRNGSetSeed%+elf_symbol_type
extern y8_ippsPRNGSetSeed%+elf_symbol_type
extern e9_ippsPRNGSetSeed%+elf_symbol_type
extern l9_ippsPRNGSetSeed%+elf_symbol_type
extern n0_ippsPRNGSetSeed%+elf_symbol_type
extern k0_ippsPRNGSetSeed%+elf_symbol_type
extern k1_ippsPRNGSetSeed%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPRNGSetSeed
.Larraddr_ippsPRNGSetSeed:
    dq m7_ippsPRNGSetSeed
    dq n8_ippsPRNGSetSeed
    dq y8_ippsPRNGSetSeed
    dq e9_ippsPRNGSetSeed
    dq l9_ippsPRNGSetSeed
    dq n0_ippsPRNGSetSeed
    dq k0_ippsPRNGSetSeed
    dq k1_ippsPRNGSetSeed

segment .text
global ippsPRNGSetSeed:function (ippsPRNGSetSeed.LEndippsPRNGSetSeed - ippsPRNGSetSeed)
.Lin_ippsPRNGSetSeed:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPRNGSetSeed:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPRNGSetSeed]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPRNGSetSeed:
