%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA512Update%+elf_symbol_type
extern n8_ippsSHA512Update%+elf_symbol_type
extern y8_ippsSHA512Update%+elf_symbol_type
extern e9_ippsSHA512Update%+elf_symbol_type
extern l9_ippsSHA512Update%+elf_symbol_type
extern n0_ippsSHA512Update%+elf_symbol_type
extern k0_ippsSHA512Update%+elf_symbol_type
extern k1_ippsSHA512Update%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA512Update
.Larraddr_ippsSHA512Update:
    dq m7_ippsSHA512Update
    dq n8_ippsSHA512Update
    dq y8_ippsSHA512Update
    dq e9_ippsSHA512Update
    dq l9_ippsSHA512Update
    dq n0_ippsSHA512Update
    dq k0_ippsSHA512Update
    dq k1_ippsSHA512Update

segment .text
global ippsSHA512Update:function (ippsSHA512Update.LEndippsSHA512Update - ippsSHA512Update)
.Lin_ippsSHA512Update:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA512Update:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA512Update]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA512Update:
