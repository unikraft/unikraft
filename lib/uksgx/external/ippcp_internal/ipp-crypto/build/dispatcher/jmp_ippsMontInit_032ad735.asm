%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMontInit%+elf_symbol_type
extern n8_ippsMontInit%+elf_symbol_type
extern y8_ippsMontInit%+elf_symbol_type
extern e9_ippsMontInit%+elf_symbol_type
extern l9_ippsMontInit%+elf_symbol_type
extern n0_ippsMontInit%+elf_symbol_type
extern k0_ippsMontInit%+elf_symbol_type
extern k1_ippsMontInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMontInit
.Larraddr_ippsMontInit:
    dq m7_ippsMontInit
    dq n8_ippsMontInit
    dq y8_ippsMontInit
    dq e9_ippsMontInit
    dq l9_ippsMontInit
    dq n0_ippsMontInit
    dq k0_ippsMontInit
    dq k1_ippsMontInit

segment .text
global ippsMontInit:function (ippsMontInit.LEndippsMontInit - ippsMontInit)
.Lin_ippsMontInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMontInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMontInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMontInit:
