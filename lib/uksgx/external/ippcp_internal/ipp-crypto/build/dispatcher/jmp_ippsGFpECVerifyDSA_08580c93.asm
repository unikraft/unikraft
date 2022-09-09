%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECVerifyDSA%+elf_symbol_type
extern n8_ippsGFpECVerifyDSA%+elf_symbol_type
extern y8_ippsGFpECVerifyDSA%+elf_symbol_type
extern e9_ippsGFpECVerifyDSA%+elf_symbol_type
extern l9_ippsGFpECVerifyDSA%+elf_symbol_type
extern n0_ippsGFpECVerifyDSA%+elf_symbol_type
extern k0_ippsGFpECVerifyDSA%+elf_symbol_type
extern k1_ippsGFpECVerifyDSA%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECVerifyDSA
.Larraddr_ippsGFpECVerifyDSA:
    dq m7_ippsGFpECVerifyDSA
    dq n8_ippsGFpECVerifyDSA
    dq y8_ippsGFpECVerifyDSA
    dq e9_ippsGFpECVerifyDSA
    dq l9_ippsGFpECVerifyDSA
    dq n0_ippsGFpECVerifyDSA
    dq k0_ippsGFpECVerifyDSA
    dq k1_ippsGFpECVerifyDSA

segment .text
global ippsGFpECVerifyDSA:function (ippsGFpECVerifyDSA.LEndippsGFpECVerifyDSA - ippsGFpECVerifyDSA)
.Lin_ippsGFpECVerifyDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECVerifyDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECVerifyDSA]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECVerifyDSA:
