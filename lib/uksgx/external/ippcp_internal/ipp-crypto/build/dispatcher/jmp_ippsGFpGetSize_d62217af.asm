%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpGetSize%+elf_symbol_type
extern n8_ippsGFpGetSize%+elf_symbol_type
extern y8_ippsGFpGetSize%+elf_symbol_type
extern e9_ippsGFpGetSize%+elf_symbol_type
extern l9_ippsGFpGetSize%+elf_symbol_type
extern n0_ippsGFpGetSize%+elf_symbol_type
extern k0_ippsGFpGetSize%+elf_symbol_type
extern k1_ippsGFpGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpGetSize
.Larraddr_ippsGFpGetSize:
    dq m7_ippsGFpGetSize
    dq n8_ippsGFpGetSize
    dq y8_ippsGFpGetSize
    dq e9_ippsGFpGetSize
    dq l9_ippsGFpGetSize
    dq n0_ippsGFpGetSize
    dq k0_ippsGFpGetSize
    dq k1_ippsGFpGetSize

segment .text
global ippsGFpGetSize:function (ippsGFpGetSize.LEndippsGFpGetSize - ippsGFpGetSize)
.Lin_ippsGFpGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpGetSize:
