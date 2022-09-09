%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPrimeInit%+elf_symbol_type
extern n8_ippsPrimeInit%+elf_symbol_type
extern y8_ippsPrimeInit%+elf_symbol_type
extern e9_ippsPrimeInit%+elf_symbol_type
extern l9_ippsPrimeInit%+elf_symbol_type
extern n0_ippsPrimeInit%+elf_symbol_type
extern k0_ippsPrimeInit%+elf_symbol_type
extern k1_ippsPrimeInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPrimeInit
.Larraddr_ippsPrimeInit:
    dq m7_ippsPrimeInit
    dq n8_ippsPrimeInit
    dq y8_ippsPrimeInit
    dq e9_ippsPrimeInit
    dq l9_ippsPrimeInit
    dq n0_ippsPrimeInit
    dq k0_ippsPrimeInit
    dq k1_ippsPrimeInit

segment .text
global ippsPrimeInit:function (ippsPrimeInit.LEndippsPrimeInit - ippsPrimeInit)
.Lin_ippsPrimeInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPrimeInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPrimeInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPrimeInit:
