%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPrimeSet%+elf_symbol_type
extern n8_ippsPrimeSet%+elf_symbol_type
extern y8_ippsPrimeSet%+elf_symbol_type
extern e9_ippsPrimeSet%+elf_symbol_type
extern l9_ippsPrimeSet%+elf_symbol_type
extern n0_ippsPrimeSet%+elf_symbol_type
extern k0_ippsPrimeSet%+elf_symbol_type
extern k1_ippsPrimeSet%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPrimeSet
.Larraddr_ippsPrimeSet:
    dq m7_ippsPrimeSet
    dq n8_ippsPrimeSet
    dq y8_ippsPrimeSet
    dq e9_ippsPrimeSet
    dq l9_ippsPrimeSet
    dq n0_ippsPrimeSet
    dq k0_ippsPrimeSet
    dq k1_ippsPrimeSet

segment .text
global ippsPrimeSet:function (ippsPrimeSet.LEndippsPrimeSet - ippsPrimeSet)
.Lin_ippsPrimeSet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPrimeSet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPrimeSet]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPrimeSet:
