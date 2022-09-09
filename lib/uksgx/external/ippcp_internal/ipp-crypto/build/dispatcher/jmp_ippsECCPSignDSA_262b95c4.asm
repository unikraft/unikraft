%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPSignDSA%+elf_symbol_type
extern n8_ippsECCPSignDSA%+elf_symbol_type
extern y8_ippsECCPSignDSA%+elf_symbol_type
extern e9_ippsECCPSignDSA%+elf_symbol_type
extern l9_ippsECCPSignDSA%+elf_symbol_type
extern n0_ippsECCPSignDSA%+elf_symbol_type
extern k0_ippsECCPSignDSA%+elf_symbol_type
extern k1_ippsECCPSignDSA%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPSignDSA
.Larraddr_ippsECCPSignDSA:
    dq m7_ippsECCPSignDSA
    dq n8_ippsECCPSignDSA
    dq y8_ippsECCPSignDSA
    dq e9_ippsECCPSignDSA
    dq l9_ippsECCPSignDSA
    dq n0_ippsECCPSignDSA
    dq k0_ippsECCPSignDSA
    dq k1_ippsECCPSignDSA

segment .text
global ippsECCPSignDSA:function (ippsECCPSignDSA.LEndippsECCPSignDSA - ippsECCPSignDSA)
.Lin_ippsECCPSignDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPSignDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPSignDSA]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPSignDSA:
