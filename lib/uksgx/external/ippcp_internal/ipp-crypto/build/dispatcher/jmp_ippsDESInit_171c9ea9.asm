%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDESInit%+elf_symbol_type
extern n8_ippsDESInit%+elf_symbol_type
extern y8_ippsDESInit%+elf_symbol_type
extern e9_ippsDESInit%+elf_symbol_type
extern l9_ippsDESInit%+elf_symbol_type
extern n0_ippsDESInit%+elf_symbol_type
extern k0_ippsDESInit%+elf_symbol_type
extern k1_ippsDESInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDESInit
.Larraddr_ippsDESInit:
    dq m7_ippsDESInit
    dq n8_ippsDESInit
    dq y8_ippsDESInit
    dq e9_ippsDESInit
    dq l9_ippsDESInit
    dq n0_ippsDESInit
    dq k0_ippsDESInit
    dq k1_ippsDESInit

segment .text
global ippsDESInit:function (ippsDESInit.LEndippsDESInit - ippsDESInit)
.Lin_ippsDESInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDESInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDESInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDESInit:
