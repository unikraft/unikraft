%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpxGetSize%+elf_symbol_type
extern n8_ippsGFpxGetSize%+elf_symbol_type
extern y8_ippsGFpxGetSize%+elf_symbol_type
extern e9_ippsGFpxGetSize%+elf_symbol_type
extern l9_ippsGFpxGetSize%+elf_symbol_type
extern n0_ippsGFpxGetSize%+elf_symbol_type
extern k0_ippsGFpxGetSize%+elf_symbol_type
extern k1_ippsGFpxGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpxGetSize
.Larraddr_ippsGFpxGetSize:
    dq m7_ippsGFpxGetSize
    dq n8_ippsGFpxGetSize
    dq y8_ippsGFpxGetSize
    dq e9_ippsGFpxGetSize
    dq l9_ippsGFpxGetSize
    dq n0_ippsGFpxGetSize
    dq k0_ippsGFpxGetSize
    dq k1_ippsGFpxGetSize

segment .text
global ippsGFpxGetSize:function (ippsGFpxGetSize.LEndippsGFpxGetSize - ippsGFpxGetSize)
.Lin_ippsGFpxGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpxGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpxGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpxGetSize:
