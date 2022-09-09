%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMD5Update%+elf_symbol_type
extern n8_ippsMD5Update%+elf_symbol_type
extern y8_ippsMD5Update%+elf_symbol_type
extern e9_ippsMD5Update%+elf_symbol_type
extern l9_ippsMD5Update%+elf_symbol_type
extern n0_ippsMD5Update%+elf_symbol_type
extern k0_ippsMD5Update%+elf_symbol_type
extern k1_ippsMD5Update%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMD5Update
.Larraddr_ippsMD5Update:
    dq m7_ippsMD5Update
    dq n8_ippsMD5Update
    dq y8_ippsMD5Update
    dq e9_ippsMD5Update
    dq l9_ippsMD5Update
    dq n0_ippsMD5Update
    dq k0_ippsMD5Update
    dq k1_ippsMD5Update

segment .text
global ippsMD5Update:function (ippsMD5Update.LEndippsMD5Update - ippsMD5Update)
.Lin_ippsMD5Update:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMD5Update:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMD5Update]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMD5Update:
