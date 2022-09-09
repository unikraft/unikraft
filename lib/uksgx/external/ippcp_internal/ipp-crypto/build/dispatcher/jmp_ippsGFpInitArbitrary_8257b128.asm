%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpInitArbitrary%+elf_symbol_type
extern n8_ippsGFpInitArbitrary%+elf_symbol_type
extern y8_ippsGFpInitArbitrary%+elf_symbol_type
extern e9_ippsGFpInitArbitrary%+elf_symbol_type
extern l9_ippsGFpInitArbitrary%+elf_symbol_type
extern n0_ippsGFpInitArbitrary%+elf_symbol_type
extern k0_ippsGFpInitArbitrary%+elf_symbol_type
extern k1_ippsGFpInitArbitrary%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpInitArbitrary
.Larraddr_ippsGFpInitArbitrary:
    dq m7_ippsGFpInitArbitrary
    dq n8_ippsGFpInitArbitrary
    dq y8_ippsGFpInitArbitrary
    dq e9_ippsGFpInitArbitrary
    dq l9_ippsGFpInitArbitrary
    dq n0_ippsGFpInitArbitrary
    dq k0_ippsGFpInitArbitrary
    dq k1_ippsGFpInitArbitrary

segment .text
global ippsGFpInitArbitrary:function (ippsGFpInitArbitrary.LEndippsGFpInitArbitrary - ippsGFpInitArbitrary)
.Lin_ippsGFpInitArbitrary:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpInitArbitrary:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpInitArbitrary]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpInitArbitrary:
