%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPrimeGet%+elf_symbol_type
extern n8_ippsPrimeGet%+elf_symbol_type
extern y8_ippsPrimeGet%+elf_symbol_type
extern e9_ippsPrimeGet%+elf_symbol_type
extern l9_ippsPrimeGet%+elf_symbol_type
extern n0_ippsPrimeGet%+elf_symbol_type
extern k0_ippsPrimeGet%+elf_symbol_type
extern k1_ippsPrimeGet%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPrimeGet
.Larraddr_ippsPrimeGet:
    dq m7_ippsPrimeGet
    dq n8_ippsPrimeGet
    dq y8_ippsPrimeGet
    dq e9_ippsPrimeGet
    dq l9_ippsPrimeGet
    dq n0_ippsPrimeGet
    dq k0_ippsPrimeGet
    dq k1_ippsPrimeGet

segment .text
global ippsPrimeGet:function (ippsPrimeGet.LEndippsPrimeGet - ippsPrimeGet)
.Lin_ippsPrimeGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPrimeGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPrimeGet]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPrimeGet:
