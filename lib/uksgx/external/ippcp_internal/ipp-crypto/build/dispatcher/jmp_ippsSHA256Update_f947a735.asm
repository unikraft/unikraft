%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA256Update%+elf_symbol_type
extern n8_ippsSHA256Update%+elf_symbol_type
extern y8_ippsSHA256Update%+elf_symbol_type
extern e9_ippsSHA256Update%+elf_symbol_type
extern l9_ippsSHA256Update%+elf_symbol_type
extern n0_ippsSHA256Update%+elf_symbol_type
extern k0_ippsSHA256Update%+elf_symbol_type
extern k1_ippsSHA256Update%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA256Update
.Larraddr_ippsSHA256Update:
    dq m7_ippsSHA256Update
    dq n8_ippsSHA256Update
    dq y8_ippsSHA256Update
    dq e9_ippsSHA256Update
    dq l9_ippsSHA256Update
    dq n0_ippsSHA256Update
    dq k0_ippsSHA256Update
    dq k1_ippsSHA256Update

segment .text
global ippsSHA256Update:function (ippsSHA256Update.LEndippsSHA256Update - ippsSHA256Update)
.Lin_ippsSHA256Update:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA256Update:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA256Update]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA256Update:
