%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMontForm%+elf_symbol_type
extern n8_ippsMontForm%+elf_symbol_type
extern y8_ippsMontForm%+elf_symbol_type
extern e9_ippsMontForm%+elf_symbol_type
extern l9_ippsMontForm%+elf_symbol_type
extern n0_ippsMontForm%+elf_symbol_type
extern k0_ippsMontForm%+elf_symbol_type
extern k1_ippsMontForm%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMontForm
.Larraddr_ippsMontForm:
    dq m7_ippsMontForm
    dq n8_ippsMontForm
    dq y8_ippsMontForm
    dq e9_ippsMontForm
    dq l9_ippsMontForm
    dq n0_ippsMontForm
    dq k0_ippsMontForm
    dq k1_ippsMontForm

segment .text
global ippsMontForm:function (ippsMontForm.LEndippsMontForm - ippsMontForm)
.Lin_ippsMontForm:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMontForm:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMontForm]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMontForm:
