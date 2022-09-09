%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESGetSize%+elf_symbol_type
extern n8_ippsAESGetSize%+elf_symbol_type
extern y8_ippsAESGetSize%+elf_symbol_type
extern e9_ippsAESGetSize%+elf_symbol_type
extern l9_ippsAESGetSize%+elf_symbol_type
extern n0_ippsAESGetSize%+elf_symbol_type
extern k0_ippsAESGetSize%+elf_symbol_type
extern k1_ippsAESGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESGetSize
.Larraddr_ippsAESGetSize:
    dq m7_ippsAESGetSize
    dq n8_ippsAESGetSize
    dq y8_ippsAESGetSize
    dq e9_ippsAESGetSize
    dq l9_ippsAESGetSize
    dq n0_ippsAESGetSize
    dq k0_ippsAESGetSize
    dq k1_ippsAESGetSize

segment .text
global ippsAESGetSize:function (ippsAESGetSize.LEndippsAESGetSize - ippsAESGetSize)
.Lin_ippsAESGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESGetSize:
