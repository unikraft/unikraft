%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern n8_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern y8_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern e9_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern l9_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern n0_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern k0_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern k1_ippsGFpECSetPointAtInfinity%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetPointAtInfinity
.Larraddr_ippsGFpECSetPointAtInfinity:
    dq m7_ippsGFpECSetPointAtInfinity
    dq n8_ippsGFpECSetPointAtInfinity
    dq y8_ippsGFpECSetPointAtInfinity
    dq e9_ippsGFpECSetPointAtInfinity
    dq l9_ippsGFpECSetPointAtInfinity
    dq n0_ippsGFpECSetPointAtInfinity
    dq k0_ippsGFpECSetPointAtInfinity
    dq k1_ippsGFpECSetPointAtInfinity

segment .text
global ippsGFpECSetPointAtInfinity:function (ippsGFpECSetPointAtInfinity.LEndippsGFpECSetPointAtInfinity - ippsGFpECSetPointAtInfinity)
.Lin_ippsGFpECSetPointAtInfinity:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetPointAtInfinity:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetPointAtInfinity]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetPointAtInfinity:
