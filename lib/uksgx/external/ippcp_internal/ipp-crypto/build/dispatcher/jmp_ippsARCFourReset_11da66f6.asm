%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsARCFourReset%+elf_symbol_type
extern n8_ippsARCFourReset%+elf_symbol_type
extern y8_ippsARCFourReset%+elf_symbol_type
extern e9_ippsARCFourReset%+elf_symbol_type
extern l9_ippsARCFourReset%+elf_symbol_type
extern n0_ippsARCFourReset%+elf_symbol_type
extern k0_ippsARCFourReset%+elf_symbol_type
extern k1_ippsARCFourReset%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsARCFourReset
.Larraddr_ippsARCFourReset:
    dq m7_ippsARCFourReset
    dq n8_ippsARCFourReset
    dq y8_ippsARCFourReset
    dq e9_ippsARCFourReset
    dq l9_ippsARCFourReset
    dq n0_ippsARCFourReset
    dq k0_ippsARCFourReset
    dq k1_ippsARCFourReset

segment .text
global ippsARCFourReset:function (ippsARCFourReset.LEndippsARCFourReset - ippsARCFourReset)
.Lin_ippsARCFourReset:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsARCFourReset:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsARCFourReset]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsARCFourReset:
