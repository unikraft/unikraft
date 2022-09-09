%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashMethodGetSize%+elf_symbol_type
extern n8_ippsHashMethodGetSize%+elf_symbol_type
extern y8_ippsHashMethodGetSize%+elf_symbol_type
extern e9_ippsHashMethodGetSize%+elf_symbol_type
extern l9_ippsHashMethodGetSize%+elf_symbol_type
extern n0_ippsHashMethodGetSize%+elf_symbol_type
extern k0_ippsHashMethodGetSize%+elf_symbol_type
extern k1_ippsHashMethodGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashMethodGetSize
.Larraddr_ippsHashMethodGetSize:
    dq m7_ippsHashMethodGetSize
    dq n8_ippsHashMethodGetSize
    dq y8_ippsHashMethodGetSize
    dq e9_ippsHashMethodGetSize
    dq l9_ippsHashMethodGetSize
    dq n0_ippsHashMethodGetSize
    dq k0_ippsHashMethodGetSize
    dq k1_ippsHashMethodGetSize

segment .text
global ippsHashMethodGetSize:function (ippsHashMethodGetSize.LEndippsHashMethodGetSize - ippsHashMethodGetSize)
.Lin_ippsHashMethodGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashMethodGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashMethodGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashMethodGetSize:
