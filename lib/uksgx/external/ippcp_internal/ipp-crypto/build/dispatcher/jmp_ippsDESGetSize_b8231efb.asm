%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDESGetSize%+elf_symbol_type
extern n8_ippsDESGetSize%+elf_symbol_type
extern y8_ippsDESGetSize%+elf_symbol_type
extern e9_ippsDESGetSize%+elf_symbol_type
extern l9_ippsDESGetSize%+elf_symbol_type
extern n0_ippsDESGetSize%+elf_symbol_type
extern k0_ippsDESGetSize%+elf_symbol_type
extern k1_ippsDESGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDESGetSize
.Larraddr_ippsDESGetSize:
    dq m7_ippsDESGetSize
    dq n8_ippsDESGetSize
    dq y8_ippsDESGetSize
    dq e9_ippsDESGetSize
    dq l9_ippsDESGetSize
    dq n0_ippsDESGetSize
    dq k0_ippsDESGetSize
    dq k1_ippsDESGetSize

segment .text
global ippsDESGetSize:function (ippsDESGetSize.LEndippsDESGetSize - ippsDESGetSize)
.Lin_ippsDESGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDESGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDESGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDESGetSize:
