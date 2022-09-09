%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMD5Final%+elf_symbol_type
extern n8_ippsMD5Final%+elf_symbol_type
extern y8_ippsMD5Final%+elf_symbol_type
extern e9_ippsMD5Final%+elf_symbol_type
extern l9_ippsMD5Final%+elf_symbol_type
extern n0_ippsMD5Final%+elf_symbol_type
extern k0_ippsMD5Final%+elf_symbol_type
extern k1_ippsMD5Final%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMD5Final
.Larraddr_ippsMD5Final:
    dq m7_ippsMD5Final
    dq n8_ippsMD5Final
    dq y8_ippsMD5Final
    dq e9_ippsMD5Final
    dq l9_ippsMD5Final
    dq n0_ippsMD5Final
    dq k0_ippsMD5Final
    dq k1_ippsMD5Final

segment .text
global ippsMD5Final:function (ippsMD5Final.LEndippsMD5Final - ippsMD5Final)
.Lin_ippsMD5Final:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMD5Final:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMD5Final]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMD5Final:
