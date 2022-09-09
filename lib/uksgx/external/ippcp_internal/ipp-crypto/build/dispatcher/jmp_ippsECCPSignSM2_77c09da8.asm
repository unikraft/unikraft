%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPSignSM2%+elf_symbol_type
extern n8_ippsECCPSignSM2%+elf_symbol_type
extern y8_ippsECCPSignSM2%+elf_symbol_type
extern e9_ippsECCPSignSM2%+elf_symbol_type
extern l9_ippsECCPSignSM2%+elf_symbol_type
extern n0_ippsECCPSignSM2%+elf_symbol_type
extern k0_ippsECCPSignSM2%+elf_symbol_type
extern k1_ippsECCPSignSM2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPSignSM2
.Larraddr_ippsECCPSignSM2:
    dq m7_ippsECCPSignSM2
    dq n8_ippsECCPSignSM2
    dq y8_ippsECCPSignSM2
    dq e9_ippsECCPSignSM2
    dq l9_ippsECCPSignSM2
    dq n0_ippsECCPSignSM2
    dq k0_ippsECCPSignSM2
    dq k1_ippsECCPSignSM2

segment .text
global ippsECCPSignSM2:function (ippsECCPSignSM2.LEndippsECCPSignSM2 - ippsECCPSignSM2)
.Lin_ippsECCPSignSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPSignSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPSignSM2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPSignSM2:
