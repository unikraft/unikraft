%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPRNGGetSeed%+elf_symbol_type
extern n8_ippsPRNGGetSeed%+elf_symbol_type
extern y8_ippsPRNGGetSeed%+elf_symbol_type
extern e9_ippsPRNGGetSeed%+elf_symbol_type
extern l9_ippsPRNGGetSeed%+elf_symbol_type
extern n0_ippsPRNGGetSeed%+elf_symbol_type
extern k0_ippsPRNGGetSeed%+elf_symbol_type
extern k1_ippsPRNGGetSeed%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPRNGGetSeed
.Larraddr_ippsPRNGGetSeed:
    dq m7_ippsPRNGGetSeed
    dq n8_ippsPRNGGetSeed
    dq y8_ippsPRNGGetSeed
    dq e9_ippsPRNGGetSeed
    dq l9_ippsPRNGGetSeed
    dq n0_ippsPRNGGetSeed
    dq k0_ippsPRNGGetSeed
    dq k1_ippsPRNGGetSeed

segment .text
global ippsPRNGGetSeed:function (ippsPRNGGetSeed.LEndippsPRNGGetSeed - ippsPRNGGetSeed)
.Lin_ippsPRNGGetSeed:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPRNGGetSeed:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPRNGGetSeed]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPRNGGetSeed:
