%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMontGet%+elf_symbol_type
extern n8_ippsMontGet%+elf_symbol_type
extern y8_ippsMontGet%+elf_symbol_type
extern e9_ippsMontGet%+elf_symbol_type
extern l9_ippsMontGet%+elf_symbol_type
extern n0_ippsMontGet%+elf_symbol_type
extern k0_ippsMontGet%+elf_symbol_type
extern k1_ippsMontGet%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMontGet
.Larraddr_ippsMontGet:
    dq m7_ippsMontGet
    dq n8_ippsMontGet
    dq y8_ippsMontGet
    dq e9_ippsMontGet
    dq l9_ippsMontGet
    dq n0_ippsMontGet
    dq k0_ippsMontGet
    dq k1_ippsMontGet

segment .text
global ippsMontGet:function (ippsMontGet.LEndippsMontGet - ippsMontGet)
.Lin_ippsMontGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMontGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMontGet]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMontGet:
