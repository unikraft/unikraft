%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpNeg%+elf_symbol_type
extern n8_ippsGFpNeg%+elf_symbol_type
extern y8_ippsGFpNeg%+elf_symbol_type
extern e9_ippsGFpNeg%+elf_symbol_type
extern l9_ippsGFpNeg%+elf_symbol_type
extern n0_ippsGFpNeg%+elf_symbol_type
extern k0_ippsGFpNeg%+elf_symbol_type
extern k1_ippsGFpNeg%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpNeg
.Larraddr_ippsGFpNeg:
    dq m7_ippsGFpNeg
    dq n8_ippsGFpNeg
    dq y8_ippsGFpNeg
    dq e9_ippsGFpNeg
    dq l9_ippsGFpNeg
    dq n0_ippsGFpNeg
    dq k0_ippsGFpNeg
    dq k1_ippsGFpNeg

segment .text
global ippsGFpNeg:function (ippsGFpNeg.LEndippsGFpNeg - ippsGFpNeg)
.Lin_ippsGFpNeg:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpNeg:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpNeg]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpNeg:
