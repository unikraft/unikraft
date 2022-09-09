%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpMultiExp%+elf_symbol_type
extern n8_ippsGFpMultiExp%+elf_symbol_type
extern y8_ippsGFpMultiExp%+elf_symbol_type
extern e9_ippsGFpMultiExp%+elf_symbol_type
extern l9_ippsGFpMultiExp%+elf_symbol_type
extern n0_ippsGFpMultiExp%+elf_symbol_type
extern k0_ippsGFpMultiExp%+elf_symbol_type
extern k1_ippsGFpMultiExp%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpMultiExp
.Larraddr_ippsGFpMultiExp:
    dq m7_ippsGFpMultiExp
    dq n8_ippsGFpMultiExp
    dq y8_ippsGFpMultiExp
    dq e9_ippsGFpMultiExp
    dq l9_ippsGFpMultiExp
    dq n0_ippsGFpMultiExp
    dq k0_ippsGFpMultiExp
    dq k1_ippsGFpMultiExp

segment .text
global ippsGFpMultiExp:function (ippsGFpMultiExp.LEndippsGFpMultiExp - ippsGFpMultiExp)
.Lin_ippsGFpMultiExp:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpMultiExp:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpMultiExp]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpMultiExp:
