%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashMessage%+elf_symbol_type
extern n8_ippsHashMessage%+elf_symbol_type
extern y8_ippsHashMessage%+elf_symbol_type
extern e9_ippsHashMessage%+elf_symbol_type
extern l9_ippsHashMessage%+elf_symbol_type
extern n0_ippsHashMessage%+elf_symbol_type
extern k0_ippsHashMessage%+elf_symbol_type
extern k1_ippsHashMessage%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashMessage
.Larraddr_ippsHashMessage:
    dq m7_ippsHashMessage
    dq n8_ippsHashMessage
    dq y8_ippsHashMessage
    dq e9_ippsHashMessage
    dq l9_ippsHashMessage
    dq n0_ippsHashMessage
    dq k0_ippsHashMessage
    dq k1_ippsHashMessage

segment .text
global ippsHashMessage:function (ippsHashMessage.LEndippsHashMessage - ippsHashMessage)
.Lin_ippsHashMessage:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashMessage:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashMessage]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashMessage:
