%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashUnpack%+elf_symbol_type
extern n8_ippsHashUnpack%+elf_symbol_type
extern y8_ippsHashUnpack%+elf_symbol_type
extern e9_ippsHashUnpack%+elf_symbol_type
extern l9_ippsHashUnpack%+elf_symbol_type
extern n0_ippsHashUnpack%+elf_symbol_type
extern k0_ippsHashUnpack%+elf_symbol_type
extern k1_ippsHashUnpack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashUnpack
.Larraddr_ippsHashUnpack:
    dq m7_ippsHashUnpack
    dq n8_ippsHashUnpack
    dq y8_ippsHashUnpack
    dq e9_ippsHashUnpack
    dq l9_ippsHashUnpack
    dq n0_ippsHashUnpack
    dq k0_ippsHashUnpack
    dq k1_ippsHashUnpack

segment .text
global ippsHashUnpack:function (ippsHashUnpack.LEndippsHashUnpack - ippsHashUnpack)
.Lin_ippsHashUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashUnpack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashUnpack:
