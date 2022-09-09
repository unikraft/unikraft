%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPRNGSetAugment%+elf_symbol_type
extern n8_ippsPRNGSetAugment%+elf_symbol_type
extern y8_ippsPRNGSetAugment%+elf_symbol_type
extern e9_ippsPRNGSetAugment%+elf_symbol_type
extern l9_ippsPRNGSetAugment%+elf_symbol_type
extern n0_ippsPRNGSetAugment%+elf_symbol_type
extern k0_ippsPRNGSetAugment%+elf_symbol_type
extern k1_ippsPRNGSetAugment%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPRNGSetAugment
.Larraddr_ippsPRNGSetAugment:
    dq m7_ippsPRNGSetAugment
    dq n8_ippsPRNGSetAugment
    dq y8_ippsPRNGSetAugment
    dq e9_ippsPRNGSetAugment
    dq l9_ippsPRNGSetAugment
    dq n0_ippsPRNGSetAugment
    dq k0_ippsPRNGSetAugment
    dq k1_ippsPRNGSetAugment

segment .text
global ippsPRNGSetAugment:function (ippsPRNGSetAugment.LEndippsPRNGSetAugment - ippsPRNGSetAugment)
.Lin_ippsPRNGSetAugment:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPRNGSetAugment:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPRNGSetAugment]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPRNGSetAugment:
