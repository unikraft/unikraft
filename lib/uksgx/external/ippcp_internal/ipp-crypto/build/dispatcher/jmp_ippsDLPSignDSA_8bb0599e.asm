%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPSignDSA%+elf_symbol_type
extern n8_ippsDLPSignDSA%+elf_symbol_type
extern y8_ippsDLPSignDSA%+elf_symbol_type
extern e9_ippsDLPSignDSA%+elf_symbol_type
extern l9_ippsDLPSignDSA%+elf_symbol_type
extern n0_ippsDLPSignDSA%+elf_symbol_type
extern k0_ippsDLPSignDSA%+elf_symbol_type
extern k1_ippsDLPSignDSA%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPSignDSA
.Larraddr_ippsDLPSignDSA:
    dq m7_ippsDLPSignDSA
    dq n8_ippsDLPSignDSA
    dq y8_ippsDLPSignDSA
    dq e9_ippsDLPSignDSA
    dq l9_ippsDLPSignDSA
    dq n0_ippsDLPSignDSA
    dq k0_ippsDLPSignDSA
    dq k1_ippsDLPSignDSA

segment .text
global ippsDLPSignDSA:function (ippsDLPSignDSA.LEndippsDLPSignDSA - ippsDLPSignDSA)
.Lin_ippsDLPSignDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPSignDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPSignDSA]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPSignDSA:
