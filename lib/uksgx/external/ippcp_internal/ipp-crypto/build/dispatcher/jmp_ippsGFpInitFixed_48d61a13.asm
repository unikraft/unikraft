%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpInitFixed%+elf_symbol_type
extern n8_ippsGFpInitFixed%+elf_symbol_type
extern y8_ippsGFpInitFixed%+elf_symbol_type
extern e9_ippsGFpInitFixed%+elf_symbol_type
extern l9_ippsGFpInitFixed%+elf_symbol_type
extern n0_ippsGFpInitFixed%+elf_symbol_type
extern k0_ippsGFpInitFixed%+elf_symbol_type
extern k1_ippsGFpInitFixed%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpInitFixed
.Larraddr_ippsGFpInitFixed:
    dq m7_ippsGFpInitFixed
    dq n8_ippsGFpInitFixed
    dq y8_ippsGFpInitFixed
    dq e9_ippsGFpInitFixed
    dq l9_ippsGFpInitFixed
    dq n0_ippsGFpInitFixed
    dq k0_ippsGFpInitFixed
    dq k1_ippsGFpInitFixed

segment .text
global ippsGFpInitFixed:function (ippsGFpInitFixed.LEndippsGFpInitFixed - ippsGFpInitFixed)
.Lin_ippsGFpInitFixed:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpInitFixed:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpInitFixed]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpInitFixed:
