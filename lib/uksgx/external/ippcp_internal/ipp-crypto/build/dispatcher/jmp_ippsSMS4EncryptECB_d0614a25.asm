%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSMS4EncryptECB%+elf_symbol_type
extern n8_ippsSMS4EncryptECB%+elf_symbol_type
extern y8_ippsSMS4EncryptECB%+elf_symbol_type
extern e9_ippsSMS4EncryptECB%+elf_symbol_type
extern l9_ippsSMS4EncryptECB%+elf_symbol_type
extern n0_ippsSMS4EncryptECB%+elf_symbol_type
extern k0_ippsSMS4EncryptECB%+elf_symbol_type
extern k1_ippsSMS4EncryptECB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSMS4EncryptECB
.Larraddr_ippsSMS4EncryptECB:
    dq m7_ippsSMS4EncryptECB
    dq n8_ippsSMS4EncryptECB
    dq y8_ippsSMS4EncryptECB
    dq e9_ippsSMS4EncryptECB
    dq l9_ippsSMS4EncryptECB
    dq n0_ippsSMS4EncryptECB
    dq k0_ippsSMS4EncryptECB
    dq k1_ippsSMS4EncryptECB

segment .text
global ippsSMS4EncryptECB:function (ippsSMS4EncryptECB.LEndippsSMS4EncryptECB - ippsSMS4EncryptECB)
.Lin_ippsSMS4EncryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSMS4EncryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSMS4EncryptECB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSMS4EncryptECB:
