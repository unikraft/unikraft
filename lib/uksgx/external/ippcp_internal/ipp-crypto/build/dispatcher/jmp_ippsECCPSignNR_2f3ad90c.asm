%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPSignNR%+elf_symbol_type
extern n8_ippsECCPSignNR%+elf_symbol_type
extern y8_ippsECCPSignNR%+elf_symbol_type
extern e9_ippsECCPSignNR%+elf_symbol_type
extern l9_ippsECCPSignNR%+elf_symbol_type
extern n0_ippsECCPSignNR%+elf_symbol_type
extern k0_ippsECCPSignNR%+elf_symbol_type
extern k1_ippsECCPSignNR%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPSignNR
.Larraddr_ippsECCPSignNR:
    dq m7_ippsECCPSignNR
    dq n8_ippsECCPSignNR
    dq y8_ippsECCPSignNR
    dq e9_ippsECCPSignNR
    dq l9_ippsECCPSignNR
    dq n0_ippsECCPSignNR
    dq k0_ippsECCPSignNR
    dq k1_ippsECCPSignNR

segment .text
global ippsECCPSignNR:function (ippsECCPSignNR.LEndippsECCPSignNR - ippsECCPSignNR)
.Lin_ippsECCPSignNR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPSignNR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPSignNR]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPSignNR:
