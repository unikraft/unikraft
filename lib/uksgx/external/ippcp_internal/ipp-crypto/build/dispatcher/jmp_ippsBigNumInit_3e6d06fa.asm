%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsBigNumInit%+elf_symbol_type
extern n8_ippsBigNumInit%+elf_symbol_type
extern y8_ippsBigNumInit%+elf_symbol_type
extern e9_ippsBigNumInit%+elf_symbol_type
extern l9_ippsBigNumInit%+elf_symbol_type
extern n0_ippsBigNumInit%+elf_symbol_type
extern k0_ippsBigNumInit%+elf_symbol_type
extern k1_ippsBigNumInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsBigNumInit
.Larraddr_ippsBigNumInit:
    dq m7_ippsBigNumInit
    dq n8_ippsBigNumInit
    dq y8_ippsBigNumInit
    dq e9_ippsBigNumInit
    dq l9_ippsBigNumInit
    dq n0_ippsBigNumInit
    dq k0_ippsBigNumInit
    dq k1_ippsBigNumInit

segment .text
global ippsBigNumInit:function (ippsBigNumInit.LEndippsBigNumInit - ippsBigNumInit)
.Lin_ippsBigNumInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsBigNumInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsBigNumInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsBigNumInit:
