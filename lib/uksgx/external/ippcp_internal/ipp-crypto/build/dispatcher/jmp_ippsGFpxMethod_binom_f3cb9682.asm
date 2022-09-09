%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpxMethod_binom%+elf_symbol_type
extern n8_ippsGFpxMethod_binom%+elf_symbol_type
extern y8_ippsGFpxMethod_binom%+elf_symbol_type
extern e9_ippsGFpxMethod_binom%+elf_symbol_type
extern l9_ippsGFpxMethod_binom%+elf_symbol_type
extern n0_ippsGFpxMethod_binom%+elf_symbol_type
extern k0_ippsGFpxMethod_binom%+elf_symbol_type
extern k1_ippsGFpxMethod_binom%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpxMethod_binom
.Larraddr_ippsGFpxMethod_binom:
    dq m7_ippsGFpxMethod_binom
    dq n8_ippsGFpxMethod_binom
    dq y8_ippsGFpxMethod_binom
    dq e9_ippsGFpxMethod_binom
    dq l9_ippsGFpxMethod_binom
    dq n0_ippsGFpxMethod_binom
    dq k0_ippsGFpxMethod_binom
    dq k1_ippsGFpxMethod_binom

segment .text
global ippsGFpxMethod_binom:function (ippsGFpxMethod_binom.LEndippsGFpxMethod_binom - ippsGFpxMethod_binom)
.Lin_ippsGFpxMethod_binom:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpxMethod_binom:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpxMethod_binom]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpxMethod_binom:
