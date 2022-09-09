%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetPointRandom%+elf_symbol_type
extern n8_ippsGFpECSetPointRandom%+elf_symbol_type
extern y8_ippsGFpECSetPointRandom%+elf_symbol_type
extern e9_ippsGFpECSetPointRandom%+elf_symbol_type
extern l9_ippsGFpECSetPointRandom%+elf_symbol_type
extern n0_ippsGFpECSetPointRandom%+elf_symbol_type
extern k0_ippsGFpECSetPointRandom%+elf_symbol_type
extern k1_ippsGFpECSetPointRandom%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetPointRandom
.Larraddr_ippsGFpECSetPointRandom:
    dq m7_ippsGFpECSetPointRandom
    dq n8_ippsGFpECSetPointRandom
    dq y8_ippsGFpECSetPointRandom
    dq e9_ippsGFpECSetPointRandom
    dq l9_ippsGFpECSetPointRandom
    dq n0_ippsGFpECSetPointRandom
    dq k0_ippsGFpECSetPointRandom
    dq k1_ippsGFpECSetPointRandom

segment .text
global ippsGFpECSetPointRandom:function (ippsGFpECSetPointRandom.LEndippsGFpECSetPointRandom - ippsGFpECSetPointRandom)
.Lin_ippsGFpECSetPointRandom:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetPointRandom:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetPointRandom]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetPointRandom:
