%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMontMul%+elf_symbol_type
extern n8_ippsMontMul%+elf_symbol_type
extern y8_ippsMontMul%+elf_symbol_type
extern e9_ippsMontMul%+elf_symbol_type
extern l9_ippsMontMul%+elf_symbol_type
extern n0_ippsMontMul%+elf_symbol_type
extern k0_ippsMontMul%+elf_symbol_type
extern k1_ippsMontMul%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMontMul
.Larraddr_ippsMontMul:
    dq m7_ippsMontMul
    dq n8_ippsMontMul
    dq y8_ippsMontMul
    dq e9_ippsMontMul
    dq l9_ippsMontMul
    dq n0_ippsMontMul
    dq k0_ippsMontMul
    dq k1_ippsMontMul

segment .text
global ippsMontMul:function (ippsMontMul.LEndippsMontMul - ippsMontMul)
.Lin_ippsMontMul:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMontMul:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMontMul]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMontMul:
