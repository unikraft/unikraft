%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashGetTag%+elf_symbol_type
extern n8_ippsHashGetTag%+elf_symbol_type
extern y8_ippsHashGetTag%+elf_symbol_type
extern e9_ippsHashGetTag%+elf_symbol_type
extern l9_ippsHashGetTag%+elf_symbol_type
extern n0_ippsHashGetTag%+elf_symbol_type
extern k0_ippsHashGetTag%+elf_symbol_type
extern k1_ippsHashGetTag%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashGetTag
.Larraddr_ippsHashGetTag:
    dq m7_ippsHashGetTag
    dq n8_ippsHashGetTag
    dq y8_ippsHashGetTag
    dq e9_ippsHashGetTag
    dq l9_ippsHashGetTag
    dq n0_ippsHashGetTag
    dq k0_ippsHashGetTag
    dq k1_ippsHashGetTag

segment .text
global ippsHashGetTag:function (ippsHashGetTag.LEndippsHashGetTag - ippsHashGetTag)
.Lin_ippsHashGetTag:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashGetTag:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashGetTag]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashGetTag:
