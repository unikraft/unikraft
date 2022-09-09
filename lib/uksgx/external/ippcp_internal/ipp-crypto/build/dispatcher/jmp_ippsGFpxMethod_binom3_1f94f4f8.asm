%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpxMethod_binom3%+elf_symbol_type
extern n8_ippsGFpxMethod_binom3%+elf_symbol_type
extern y8_ippsGFpxMethod_binom3%+elf_symbol_type
extern e9_ippsGFpxMethod_binom3%+elf_symbol_type
extern l9_ippsGFpxMethod_binom3%+elf_symbol_type
extern n0_ippsGFpxMethod_binom3%+elf_symbol_type
extern k0_ippsGFpxMethod_binom3%+elf_symbol_type
extern k1_ippsGFpxMethod_binom3%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpxMethod_binom3
.Larraddr_ippsGFpxMethod_binom3:
    dq m7_ippsGFpxMethod_binom3
    dq n8_ippsGFpxMethod_binom3
    dq y8_ippsGFpxMethod_binom3
    dq e9_ippsGFpxMethod_binom3
    dq l9_ippsGFpxMethod_binom3
    dq n0_ippsGFpxMethod_binom3
    dq k0_ippsGFpxMethod_binom3
    dq k1_ippsGFpxMethod_binom3

segment .text
global ippsGFpxMethod_binom3:function (ippsGFpxMethod_binom3.LEndippsGFpxMethod_binom3 - ippsGFpxMethod_binom3)
.Lin_ippsGFpxMethod_binom3:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpxMethod_binom3:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpxMethod_binom3]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpxMethod_binom3:
