%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPrimeGetSize%+elf_symbol_type
extern n8_ippsPrimeGetSize%+elf_symbol_type
extern y8_ippsPrimeGetSize%+elf_symbol_type
extern e9_ippsPrimeGetSize%+elf_symbol_type
extern l9_ippsPrimeGetSize%+elf_symbol_type
extern n0_ippsPrimeGetSize%+elf_symbol_type
extern k0_ippsPrimeGetSize%+elf_symbol_type
extern k1_ippsPrimeGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPrimeGetSize
.Larraddr_ippsPrimeGetSize:
    dq m7_ippsPrimeGetSize
    dq n8_ippsPrimeGetSize
    dq y8_ippsPrimeGetSize
    dq e9_ippsPrimeGetSize
    dq l9_ippsPrimeGetSize
    dq n0_ippsPrimeGetSize
    dq k0_ippsPrimeGetSize
    dq k1_ippsPrimeGetSize

segment .text
global ippsPrimeGetSize:function (ippsPrimeGetSize.LEndippsPrimeGetSize - ippsPrimeGetSize)
.Lin_ippsPrimeGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPrimeGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPrimeGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPrimeGetSize:
