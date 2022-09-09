%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpExp%+elf_symbol_type
extern n8_ippsGFpExp%+elf_symbol_type
extern y8_ippsGFpExp%+elf_symbol_type
extern e9_ippsGFpExp%+elf_symbol_type
extern l9_ippsGFpExp%+elf_symbol_type
extern n0_ippsGFpExp%+elf_symbol_type
extern k0_ippsGFpExp%+elf_symbol_type
extern k1_ippsGFpExp%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpExp
.Larraddr_ippsGFpExp:
    dq m7_ippsGFpExp
    dq n8_ippsGFpExp
    dq y8_ippsGFpExp
    dq e9_ippsGFpExp
    dq l9_ippsGFpExp
    dq n0_ippsGFpExp
    dq k0_ippsGFpExp
    dq k1_ippsGFpExp

segment .text
global ippsGFpExp:function (ippsGFpExp.LEndippsGFpExp - ippsGFpExp)
.Lin_ippsGFpExp:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpExp:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpExp]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpExp:
