%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA224Init%+elf_symbol_type
extern n8_ippsSHA224Init%+elf_symbol_type
extern y8_ippsSHA224Init%+elf_symbol_type
extern e9_ippsSHA224Init%+elf_symbol_type
extern l9_ippsSHA224Init%+elf_symbol_type
extern n0_ippsSHA224Init%+elf_symbol_type
extern k0_ippsSHA224Init%+elf_symbol_type
extern k1_ippsSHA224Init%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA224Init
.Larraddr_ippsSHA224Init:
    dq m7_ippsSHA224Init
    dq n8_ippsSHA224Init
    dq y8_ippsSHA224Init
    dq e9_ippsSHA224Init
    dq l9_ippsSHA224Init
    dq n0_ippsSHA224Init
    dq k0_ippsSHA224Init
    dq k1_ippsSHA224Init

segment .text
global ippsSHA224Init:function (ippsSHA224Init.LEndippsSHA224Init - ippsSHA224Init)
.Lin_ippsSHA224Init:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA224Init:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA224Init]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA224Init:
