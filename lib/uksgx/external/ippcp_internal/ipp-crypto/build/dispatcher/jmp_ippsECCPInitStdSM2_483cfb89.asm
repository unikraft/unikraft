%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPInitStdSM2%+elf_symbol_type
extern n8_ippsECCPInitStdSM2%+elf_symbol_type
extern y8_ippsECCPInitStdSM2%+elf_symbol_type
extern e9_ippsECCPInitStdSM2%+elf_symbol_type
extern l9_ippsECCPInitStdSM2%+elf_symbol_type
extern n0_ippsECCPInitStdSM2%+elf_symbol_type
extern k0_ippsECCPInitStdSM2%+elf_symbol_type
extern k1_ippsECCPInitStdSM2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPInitStdSM2
.Larraddr_ippsECCPInitStdSM2:
    dq m7_ippsECCPInitStdSM2
    dq n8_ippsECCPInitStdSM2
    dq y8_ippsECCPInitStdSM2
    dq e9_ippsECCPInitStdSM2
    dq l9_ippsECCPInitStdSM2
    dq n0_ippsECCPInitStdSM2
    dq k0_ippsECCPInitStdSM2
    dq k1_ippsECCPInitStdSM2

segment .text
global ippsECCPInitStdSM2:function (ippsECCPInitStdSM2.LEndippsECCPInitStdSM2 - ippsECCPInitStdSM2)
.Lin_ippsECCPInitStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPInitStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPInitStdSM2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPInitStdSM2:
