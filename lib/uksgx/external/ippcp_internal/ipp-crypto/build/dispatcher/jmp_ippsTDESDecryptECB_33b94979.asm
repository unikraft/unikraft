%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTDESDecryptECB%+elf_symbol_type
extern n8_ippsTDESDecryptECB%+elf_symbol_type
extern y8_ippsTDESDecryptECB%+elf_symbol_type
extern e9_ippsTDESDecryptECB%+elf_symbol_type
extern l9_ippsTDESDecryptECB%+elf_symbol_type
extern n0_ippsTDESDecryptECB%+elf_symbol_type
extern k0_ippsTDESDecryptECB%+elf_symbol_type
extern k1_ippsTDESDecryptECB%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTDESDecryptECB
.Larraddr_ippsTDESDecryptECB:
    dq m7_ippsTDESDecryptECB
    dq n8_ippsTDESDecryptECB
    dq y8_ippsTDESDecryptECB
    dq e9_ippsTDESDecryptECB
    dq l9_ippsTDESDecryptECB
    dq n0_ippsTDESDecryptECB
    dq k0_ippsTDESDecryptECB
    dq k1_ippsTDESDecryptECB

segment .text
global ippsTDESDecryptECB:function (ippsTDESDecryptECB.LEndippsTDESDecryptECB - ippsTDESDecryptECB)
.Lin_ippsTDESDecryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTDESDecryptECB:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTDESDecryptECB]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTDESDecryptECB:
