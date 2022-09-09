%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPInit%+elf_symbol_type
extern n8_ippsDLPInit%+elf_symbol_type
extern y8_ippsDLPInit%+elf_symbol_type
extern e9_ippsDLPInit%+elf_symbol_type
extern l9_ippsDLPInit%+elf_symbol_type
extern n0_ippsDLPInit%+elf_symbol_type
extern k0_ippsDLPInit%+elf_symbol_type
extern k1_ippsDLPInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPInit
.Larraddr_ippsDLPInit:
    dq m7_ippsDLPInit
    dq n8_ippsDLPInit
    dq y8_ippsDLPInit
    dq e9_ippsDLPInit
    dq l9_ippsDLPInit
    dq n0_ippsDLPInit
    dq k0_ippsDLPInit
    dq k1_ippsDLPInit

segment .text
global ippsDLPInit:function (ippsDLPInit.LEndippsDLPInit - ippsDLPInit)
.Lin_ippsDLPInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPInit:
