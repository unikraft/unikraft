%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpSqr%+elf_symbol_type
extern n8_ippsGFpSqr%+elf_symbol_type
extern y8_ippsGFpSqr%+elf_symbol_type
extern e9_ippsGFpSqr%+elf_symbol_type
extern l9_ippsGFpSqr%+elf_symbol_type
extern n0_ippsGFpSqr%+elf_symbol_type
extern k0_ippsGFpSqr%+elf_symbol_type
extern k1_ippsGFpSqr%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpSqr
.Larraddr_ippsGFpSqr:
    dq m7_ippsGFpSqr
    dq n8_ippsGFpSqr
    dq y8_ippsGFpSqr
    dq e9_ippsGFpSqr
    dq l9_ippsGFpSqr
    dq n0_ippsGFpSqr
    dq k0_ippsGFpSqr
    dq k1_ippsGFpSqr

segment .text
global ippsGFpSqr:function (ippsGFpSqr.LEndippsGFpSqr - ippsGFpSqr)
.Lin_ippsGFpSqr:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpSqr:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpSqr]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpSqr:
