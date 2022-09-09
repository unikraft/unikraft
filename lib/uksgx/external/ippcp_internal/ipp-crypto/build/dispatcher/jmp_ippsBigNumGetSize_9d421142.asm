%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsBigNumGetSize%+elf_symbol_type
extern n8_ippsBigNumGetSize%+elf_symbol_type
extern y8_ippsBigNumGetSize%+elf_symbol_type
extern e9_ippsBigNumGetSize%+elf_symbol_type
extern l9_ippsBigNumGetSize%+elf_symbol_type
extern n0_ippsBigNumGetSize%+elf_symbol_type
extern k0_ippsBigNumGetSize%+elf_symbol_type
extern k1_ippsBigNumGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsBigNumGetSize
.Larraddr_ippsBigNumGetSize:
    dq m7_ippsBigNumGetSize
    dq n8_ippsBigNumGetSize
    dq y8_ippsBigNumGetSize
    dq e9_ippsBigNumGetSize
    dq l9_ippsBigNumGetSize
    dq n0_ippsBigNumGetSize
    dq k0_ippsBigNumGetSize
    dq k1_ippsBigNumGetSize

segment .text
global ippsBigNumGetSize:function (ippsBigNumGetSize.LEndippsBigNumGetSize - ippsBigNumGetSize)
.Lin_ippsBigNumGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsBigNumGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsBigNumGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsBigNumGetSize:
