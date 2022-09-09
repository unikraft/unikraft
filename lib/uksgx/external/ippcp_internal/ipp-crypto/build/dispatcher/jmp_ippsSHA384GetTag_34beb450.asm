%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA384GetTag%+elf_symbol_type
extern n8_ippsSHA384GetTag%+elf_symbol_type
extern y8_ippsSHA384GetTag%+elf_symbol_type
extern e9_ippsSHA384GetTag%+elf_symbol_type
extern l9_ippsSHA384GetTag%+elf_symbol_type
extern n0_ippsSHA384GetTag%+elf_symbol_type
extern k0_ippsSHA384GetTag%+elf_symbol_type
extern k1_ippsSHA384GetTag%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA384GetTag
.Larraddr_ippsSHA384GetTag:
    dq m7_ippsSHA384GetTag
    dq n8_ippsSHA384GetTag
    dq y8_ippsSHA384GetTag
    dq e9_ippsSHA384GetTag
    dq l9_ippsSHA384GetTag
    dq n0_ippsSHA384GetTag
    dq k0_ippsSHA384GetTag
    dq k1_ippsSHA384GetTag

segment .text
global ippsSHA384GetTag:function (ippsSHA384GetTag.LEndippsSHA384GetTag - ippsSHA384GetTag)
.Lin_ippsSHA384GetTag:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA384GetTag:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA384GetTag]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA384GetTag:
