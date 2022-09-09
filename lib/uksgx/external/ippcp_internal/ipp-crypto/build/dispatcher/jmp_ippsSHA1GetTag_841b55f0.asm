%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA1GetTag%+elf_symbol_type
extern n8_ippsSHA1GetTag%+elf_symbol_type
extern y8_ippsSHA1GetTag%+elf_symbol_type
extern e9_ippsSHA1GetTag%+elf_symbol_type
extern l9_ippsSHA1GetTag%+elf_symbol_type
extern n0_ippsSHA1GetTag%+elf_symbol_type
extern k0_ippsSHA1GetTag%+elf_symbol_type
extern k1_ippsSHA1GetTag%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA1GetTag
.Larraddr_ippsSHA1GetTag:
    dq m7_ippsSHA1GetTag
    dq n8_ippsSHA1GetTag
    dq y8_ippsSHA1GetTag
    dq e9_ippsSHA1GetTag
    dq l9_ippsSHA1GetTag
    dq n0_ippsSHA1GetTag
    dq k0_ippsSHA1GetTag
    dq k1_ippsSHA1GetTag

segment .text
global ippsSHA1GetTag:function (ippsSHA1GetTag.LEndippsSHA1GetTag - ippsSHA1GetTag)
.Lin_ippsSHA1GetTag:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA1GetTag:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA1GetTag]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA1GetTag:
