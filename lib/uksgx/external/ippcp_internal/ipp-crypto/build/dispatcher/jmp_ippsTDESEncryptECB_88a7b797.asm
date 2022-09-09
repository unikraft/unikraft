%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTDESEncryptECB%+elf_symbol_type
extern n8_ippsTDESEncryptECB%+elf_symbol_type
extern y8_ippsTDESEncryptECB%+elf_symbol_type
extern e9_ippsTDESEncryptECB%+elf_symbol_type
extern l9_ippsTDESEncryptECB%+elf_symbol_type
extern n0_ippsTDESEncryptECB%+elf_symbol_type
extern k0_ippsTDESEncryptECB%+elf_symbol_type
extern k1_ippsTDESEncryptECB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTDESEncryptECB
.Larraddr_ippsTDESEncryptECB:
    dq m7_ippsTDESEncryptECB
    dq n8_ippsTDESEncryptECB
    dq y8_ippsTDESEncryptECB
    dq e9_ippsTDESEncryptECB
    dq l9_ippsTDESEncryptECB
    dq n0_ippsTDESEncryptECB
    dq k0_ippsTDESEncryptECB
    dq k1_ippsTDESEncryptECB

segment .text
global ippsTDESEncryptECB:function (ippsTDESEncryptECB.LEndippsTDESEncryptECB - ippsTDESEncryptECB)
.Lin_ippsTDESEncryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTDESEncryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTDESEncryptECB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTDESEncryptECB:
