%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPrimeTest%+elf_symbol_type
extern n8_ippsPrimeTest%+elf_symbol_type
extern y8_ippsPrimeTest%+elf_symbol_type
extern e9_ippsPrimeTest%+elf_symbol_type
extern l9_ippsPrimeTest%+elf_symbol_type
extern n0_ippsPrimeTest%+elf_symbol_type
extern k0_ippsPrimeTest%+elf_symbol_type
extern k1_ippsPrimeTest%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPrimeTest
.Larraddr_ippsPrimeTest:
    dq m7_ippsPrimeTest
    dq n8_ippsPrimeTest
    dq y8_ippsPrimeTest
    dq e9_ippsPrimeTest
    dq l9_ippsPrimeTest
    dq n0_ippsPrimeTest
    dq k0_ippsPrimeTest
    dq k1_ippsPrimeTest

segment .text
global ippsPrimeTest:function (ippsPrimeTest.LEndippsPrimeTest - ippsPrimeTest)
.Lin_ippsPrimeTest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPrimeTest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPrimeTest]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPrimeTest:
