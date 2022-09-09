%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpElementGetSize%+elf_symbol_type
extern n8_ippsGFpElementGetSize%+elf_symbol_type
extern y8_ippsGFpElementGetSize%+elf_symbol_type
extern e9_ippsGFpElementGetSize%+elf_symbol_type
extern l9_ippsGFpElementGetSize%+elf_symbol_type
extern n0_ippsGFpElementGetSize%+elf_symbol_type
extern k0_ippsGFpElementGetSize%+elf_symbol_type
extern k1_ippsGFpElementGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpElementGetSize
.Larraddr_ippsGFpElementGetSize:
    dq m7_ippsGFpElementGetSize
    dq n8_ippsGFpElementGetSize
    dq y8_ippsGFpElementGetSize
    dq e9_ippsGFpElementGetSize
    dq l9_ippsGFpElementGetSize
    dq n0_ippsGFpElementGetSize
    dq k0_ippsGFpElementGetSize
    dq k1_ippsGFpElementGetSize

segment .text
global ippsGFpElementGetSize:function (ippsGFpElementGetSize.LEndippsGFpElementGetSize - ippsGFpElementGetSize)
.Lin_ippsGFpElementGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpElementGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpElementGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpElementGetSize:
