%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPInitStd128r2%+elf_symbol_type
extern n8_ippsECCPInitStd128r2%+elf_symbol_type
extern y8_ippsECCPInitStd128r2%+elf_symbol_type
extern e9_ippsECCPInitStd128r2%+elf_symbol_type
extern l9_ippsECCPInitStd128r2%+elf_symbol_type
extern n0_ippsECCPInitStd128r2%+elf_symbol_type
extern k0_ippsECCPInitStd128r2%+elf_symbol_type
extern k1_ippsECCPInitStd128r2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPInitStd128r2
.Larraddr_ippsECCPInitStd128r2:
    dq m7_ippsECCPInitStd128r2
    dq n8_ippsECCPInitStd128r2
    dq y8_ippsECCPInitStd128r2
    dq e9_ippsECCPInitStd128r2
    dq l9_ippsECCPInitStd128r2
    dq n0_ippsECCPInitStd128r2
    dq k0_ippsECCPInitStd128r2
    dq k1_ippsECCPInitStd128r2

segment .text
global ippsECCPInitStd128r2:function (ippsECCPInitStd128r2.LEndippsECCPInitStd128r2 - ippsECCPInitStd128r2)
.Lin_ippsECCPInitStd128r2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPInitStd128r2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPInitStd128r2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPInitStd128r2:
