%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern n8_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern y8_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern e9_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern l9_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern n0_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern k0_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern k1_ippsGFpECSharedSecretDHC%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSharedSecretDHC
.Larraddr_ippsGFpECSharedSecretDHC:
    dq m7_ippsGFpECSharedSecretDHC
    dq n8_ippsGFpECSharedSecretDHC
    dq y8_ippsGFpECSharedSecretDHC
    dq e9_ippsGFpECSharedSecretDHC
    dq l9_ippsGFpECSharedSecretDHC
    dq n0_ippsGFpECSharedSecretDHC
    dq k0_ippsGFpECSharedSecretDHC
    dq k1_ippsGFpECSharedSecretDHC

segment .text
global ippsGFpECSharedSecretDHC:function (ippsGFpECSharedSecretDHC.LEndippsGFpECSharedSecretDHC - ippsGFpECSharedSecretDHC)
.Lin_ippsGFpECSharedSecretDHC:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSharedSecretDHC:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSharedSecretDHC]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSharedSecretDHC:
