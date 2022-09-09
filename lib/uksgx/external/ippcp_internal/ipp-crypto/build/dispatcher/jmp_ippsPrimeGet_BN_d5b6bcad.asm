%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPrimeGet_BN%+elf_symbol_type
extern n8_ippsPrimeGet_BN%+elf_symbol_type
extern y8_ippsPrimeGet_BN%+elf_symbol_type
extern e9_ippsPrimeGet_BN%+elf_symbol_type
extern l9_ippsPrimeGet_BN%+elf_symbol_type
extern n0_ippsPrimeGet_BN%+elf_symbol_type
extern k0_ippsPrimeGet_BN%+elf_symbol_type
extern k1_ippsPrimeGet_BN%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPrimeGet_BN
.Larraddr_ippsPrimeGet_BN:
    dq m7_ippsPrimeGet_BN
    dq n8_ippsPrimeGet_BN
    dq y8_ippsPrimeGet_BN
    dq e9_ippsPrimeGet_BN
    dq l9_ippsPrimeGet_BN
    dq n0_ippsPrimeGet_BN
    dq k0_ippsPrimeGet_BN
    dq k1_ippsPrimeGet_BN

segment .text
global ippsPrimeGet_BN:function (ippsPrimeGet_BN.LEndippsPrimeGet_BN - ippsPrimeGet_BN)
.Lin_ippsPrimeGet_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPrimeGet_BN:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPrimeGet_BN]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPrimeGet_BN:
