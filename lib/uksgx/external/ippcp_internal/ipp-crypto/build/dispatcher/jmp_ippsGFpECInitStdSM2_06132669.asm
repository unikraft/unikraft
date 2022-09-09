%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECInitStdSM2%+elf_symbol_type
extern n8_ippsGFpECInitStdSM2%+elf_symbol_type
extern y8_ippsGFpECInitStdSM2%+elf_symbol_type
extern e9_ippsGFpECInitStdSM2%+elf_symbol_type
extern l9_ippsGFpECInitStdSM2%+elf_symbol_type
extern n0_ippsGFpECInitStdSM2%+elf_symbol_type
extern k0_ippsGFpECInitStdSM2%+elf_symbol_type
extern k1_ippsGFpECInitStdSM2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECInitStdSM2
.Larraddr_ippsGFpECInitStdSM2:
    dq m7_ippsGFpECInitStdSM2
    dq n8_ippsGFpECInitStdSM2
    dq y8_ippsGFpECInitStdSM2
    dq e9_ippsGFpECInitStdSM2
    dq l9_ippsGFpECInitStdSM2
    dq n0_ippsGFpECInitStdSM2
    dq k0_ippsGFpECInitStdSM2
    dq k1_ippsGFpECInitStdSM2

segment .text
global ippsGFpECInitStdSM2:function (ippsGFpECInitStdSM2.LEndippsGFpECInitStdSM2 - ippsGFpECInitStdSM2)
.Lin_ippsGFpECInitStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECInitStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECInitStdSM2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECInitStdSM2:
