%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPVerifyNR%+elf_symbol_type
extern n8_ippsECCPVerifyNR%+elf_symbol_type
extern y8_ippsECCPVerifyNR%+elf_symbol_type
extern e9_ippsECCPVerifyNR%+elf_symbol_type
extern l9_ippsECCPVerifyNR%+elf_symbol_type
extern n0_ippsECCPVerifyNR%+elf_symbol_type
extern k0_ippsECCPVerifyNR%+elf_symbol_type
extern k1_ippsECCPVerifyNR%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPVerifyNR
.Larraddr_ippsECCPVerifyNR:
    dq m7_ippsECCPVerifyNR
    dq n8_ippsECCPVerifyNR
    dq y8_ippsECCPVerifyNR
    dq e9_ippsECCPVerifyNR
    dq l9_ippsECCPVerifyNR
    dq n0_ippsECCPVerifyNR
    dq k0_ippsECCPVerifyNR
    dq k1_ippsECCPVerifyNR

segment .text
global ippsECCPVerifyNR:function (ippsECCPVerifyNR.LEndippsECCPVerifyNR - ippsECCPVerifyNR)
.Lin_ippsECCPVerifyNR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPVerifyNR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPVerifyNR]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPVerifyNR:
