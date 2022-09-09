%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSMS4Init%+elf_symbol_type
extern n8_ippsSMS4Init%+elf_symbol_type
extern y8_ippsSMS4Init%+elf_symbol_type
extern e9_ippsSMS4Init%+elf_symbol_type
extern l9_ippsSMS4Init%+elf_symbol_type
extern n0_ippsSMS4Init%+elf_symbol_type
extern k0_ippsSMS4Init%+elf_symbol_type
extern k1_ippsSMS4Init%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSMS4Init
.Larraddr_ippsSMS4Init:
    dq m7_ippsSMS4Init
    dq n8_ippsSMS4Init
    dq y8_ippsSMS4Init
    dq e9_ippsSMS4Init
    dq l9_ippsSMS4Init
    dq n0_ippsSMS4Init
    dq k0_ippsSMS4Init
    dq k1_ippsSMS4Init

segment .text
global ippsSMS4Init:function (ippsSMS4Init.LEndippsSMS4Init - ippsSMS4Init)
.Lin_ippsSMS4Init:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSMS4Init:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSMS4Init]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSMS4Init:
