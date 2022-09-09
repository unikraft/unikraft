%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPSharedSecretDH%+elf_symbol_type
extern n8_ippsECCPSharedSecretDH%+elf_symbol_type
extern y8_ippsECCPSharedSecretDH%+elf_symbol_type
extern e9_ippsECCPSharedSecretDH%+elf_symbol_type
extern l9_ippsECCPSharedSecretDH%+elf_symbol_type
extern n0_ippsECCPSharedSecretDH%+elf_symbol_type
extern k0_ippsECCPSharedSecretDH%+elf_symbol_type
extern k1_ippsECCPSharedSecretDH%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPSharedSecretDH
.Larraddr_ippsECCPSharedSecretDH:
    dq m7_ippsECCPSharedSecretDH
    dq n8_ippsECCPSharedSecretDH
    dq y8_ippsECCPSharedSecretDH
    dq e9_ippsECCPSharedSecretDH
    dq l9_ippsECCPSharedSecretDH
    dq n0_ippsECCPSharedSecretDH
    dq k0_ippsECCPSharedSecretDH
    dq k1_ippsECCPSharedSecretDH

segment .text
global ippsECCPSharedSecretDH:function (ippsECCPSharedSecretDH.LEndippsECCPSharedSecretDH - ippsECCPSharedSecretDH)
.Lin_ippsECCPSharedSecretDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPSharedSecretDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPSharedSecretDH]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPSharedSecretDH:
