%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPGetSize%+elf_symbol_type
extern n8_ippsDLPGetSize%+elf_symbol_type
extern y8_ippsDLPGetSize%+elf_symbol_type
extern e9_ippsDLPGetSize%+elf_symbol_type
extern l9_ippsDLPGetSize%+elf_symbol_type
extern n0_ippsDLPGetSize%+elf_symbol_type
extern k0_ippsDLPGetSize%+elf_symbol_type
extern k1_ippsDLPGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPGetSize
.Larraddr_ippsDLPGetSize:
    dq m7_ippsDLPGetSize
    dq n8_ippsDLPGetSize
    dq y8_ippsDLPGetSize
    dq e9_ippsDLPGetSize
    dq l9_ippsDLPGetSize
    dq n0_ippsDLPGetSize
    dq k0_ippsDLPGetSize
    dq k1_ippsDLPGetSize

segment .text
global ippsDLPGetSize:function (ippsDLPGetSize.LEndippsDLPGetSize - ippsDLPGetSize)
.Lin_ippsDLPGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPGetSize:
