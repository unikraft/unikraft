%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSharedSecretDH%+elf_symbol_type
extern n8_ippsGFpECSharedSecretDH%+elf_symbol_type
extern y8_ippsGFpECSharedSecretDH%+elf_symbol_type
extern e9_ippsGFpECSharedSecretDH%+elf_symbol_type
extern l9_ippsGFpECSharedSecretDH%+elf_symbol_type
extern n0_ippsGFpECSharedSecretDH%+elf_symbol_type
extern k0_ippsGFpECSharedSecretDH%+elf_symbol_type
extern k1_ippsGFpECSharedSecretDH%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSharedSecretDH
.Larraddr_ippsGFpECSharedSecretDH:
    dq m7_ippsGFpECSharedSecretDH
    dq n8_ippsGFpECSharedSecretDH
    dq y8_ippsGFpECSharedSecretDH
    dq e9_ippsGFpECSharedSecretDH
    dq l9_ippsGFpECSharedSecretDH
    dq n0_ippsGFpECSharedSecretDH
    dq k0_ippsGFpECSharedSecretDH
    dq k1_ippsGFpECSharedSecretDH

segment .text
global ippsGFpECSharedSecretDH:function (ippsGFpECSharedSecretDH.LEndippsGFpECSharedSecretDH - ippsGFpECSharedSecretDH)
.Lin_ippsGFpECSharedSecretDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSharedSecretDH:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSharedSecretDH]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSharedSecretDH:
