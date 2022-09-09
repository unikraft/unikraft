%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPVerifyDSA%+elf_symbol_type
extern n8_ippsDLPVerifyDSA%+elf_symbol_type
extern y8_ippsDLPVerifyDSA%+elf_symbol_type
extern e9_ippsDLPVerifyDSA%+elf_symbol_type
extern l9_ippsDLPVerifyDSA%+elf_symbol_type
extern n0_ippsDLPVerifyDSA%+elf_symbol_type
extern k0_ippsDLPVerifyDSA%+elf_symbol_type
extern k1_ippsDLPVerifyDSA%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPVerifyDSA
.Larraddr_ippsDLPVerifyDSA:
    dq m7_ippsDLPVerifyDSA
    dq n8_ippsDLPVerifyDSA
    dq y8_ippsDLPVerifyDSA
    dq e9_ippsDLPVerifyDSA
    dq l9_ippsDLPVerifyDSA
    dq n0_ippsDLPVerifyDSA
    dq k0_ippsDLPVerifyDSA
    dq k1_ippsDLPVerifyDSA

segment .text
global ippsDLPVerifyDSA:function (ippsDLPVerifyDSA.LEndippsDLPVerifyDSA - ippsDLPVerifyDSA)
.Lin_ippsDLPVerifyDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPVerifyDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPVerifyDSA]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPVerifyDSA:
