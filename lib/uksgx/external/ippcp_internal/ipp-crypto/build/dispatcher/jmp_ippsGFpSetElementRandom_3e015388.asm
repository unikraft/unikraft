%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpSetElementRandom%+elf_symbol_type
extern n8_ippsGFpSetElementRandom%+elf_symbol_type
extern y8_ippsGFpSetElementRandom%+elf_symbol_type
extern e9_ippsGFpSetElementRandom%+elf_symbol_type
extern l9_ippsGFpSetElementRandom%+elf_symbol_type
extern n0_ippsGFpSetElementRandom%+elf_symbol_type
extern k0_ippsGFpSetElementRandom%+elf_symbol_type
extern k1_ippsGFpSetElementRandom%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpSetElementRandom
.Larraddr_ippsGFpSetElementRandom:
    dq m7_ippsGFpSetElementRandom
    dq n8_ippsGFpSetElementRandom
    dq y8_ippsGFpSetElementRandom
    dq e9_ippsGFpSetElementRandom
    dq l9_ippsGFpSetElementRandom
    dq n0_ippsGFpSetElementRandom
    dq k0_ippsGFpSetElementRandom
    dq k1_ippsGFpSetElementRandom

segment .text
global ippsGFpSetElementRandom:function (ippsGFpSetElementRandom.LEndippsGFpSetElementRandom - ippsGFpSetElementRandom)
.Lin_ippsGFpSetElementRandom:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpSetElementRandom:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpSetElementRandom]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpSetElementRandom:
