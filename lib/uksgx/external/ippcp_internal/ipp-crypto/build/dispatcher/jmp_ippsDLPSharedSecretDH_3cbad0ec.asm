%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPSharedSecretDH%+elf_symbol_type
extern n8_ippsDLPSharedSecretDH%+elf_symbol_type
extern y8_ippsDLPSharedSecretDH%+elf_symbol_type
extern e9_ippsDLPSharedSecretDH%+elf_symbol_type
extern l9_ippsDLPSharedSecretDH%+elf_symbol_type
extern n0_ippsDLPSharedSecretDH%+elf_symbol_type
extern k0_ippsDLPSharedSecretDH%+elf_symbol_type
extern k1_ippsDLPSharedSecretDH%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPSharedSecretDH
.Larraddr_ippsDLPSharedSecretDH:
    dq m7_ippsDLPSharedSecretDH
    dq n8_ippsDLPSharedSecretDH
    dq y8_ippsDLPSharedSecretDH
    dq e9_ippsDLPSharedSecretDH
    dq l9_ippsDLPSharedSecretDH
    dq n0_ippsDLPSharedSecretDH
    dq k0_ippsDLPSharedSecretDH
    dq k1_ippsDLPSharedSecretDH

segment .text
global ippsDLPSharedSecretDH:function (ippsDLPSharedSecretDH.LEndippsDLPSharedSecretDH - ippsDLPSharedSecretDH)
.Lin_ippsDLPSharedSecretDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPSharedSecretDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPSharedSecretDH]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPSharedSecretDH:
