%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashUpdate%+elf_symbol_type
extern n8_ippsHashUpdate%+elf_symbol_type
extern y8_ippsHashUpdate%+elf_symbol_type
extern e9_ippsHashUpdate%+elf_symbol_type
extern l9_ippsHashUpdate%+elf_symbol_type
extern n0_ippsHashUpdate%+elf_symbol_type
extern k0_ippsHashUpdate%+elf_symbol_type
extern k1_ippsHashUpdate%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashUpdate
.Larraddr_ippsHashUpdate:
    dq m7_ippsHashUpdate
    dq n8_ippsHashUpdate
    dq y8_ippsHashUpdate
    dq e9_ippsHashUpdate
    dq l9_ippsHashUpdate
    dq n0_ippsHashUpdate
    dq k0_ippsHashUpdate
    dq k1_ippsHashUpdate

segment .text
global ippsHashUpdate:function (ippsHashUpdate.LEndippsHashUpdate - ippsHashUpdate)
.Lin_ippsHashUpdate:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashUpdate:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashUpdate]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashUpdate:
