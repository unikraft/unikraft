%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSignDSA%+elf_symbol_type
extern n8_ippsGFpECSignDSA%+elf_symbol_type
extern y8_ippsGFpECSignDSA%+elf_symbol_type
extern e9_ippsGFpECSignDSA%+elf_symbol_type
extern l9_ippsGFpECSignDSA%+elf_symbol_type
extern n0_ippsGFpECSignDSA%+elf_symbol_type
extern k0_ippsGFpECSignDSA%+elf_symbol_type
extern k1_ippsGFpECSignDSA%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSignDSA
.Larraddr_ippsGFpECSignDSA:
    dq m7_ippsGFpECSignDSA
    dq n8_ippsGFpECSignDSA
    dq y8_ippsGFpECSignDSA
    dq e9_ippsGFpECSignDSA
    dq l9_ippsGFpECSignDSA
    dq n0_ippsGFpECSignDSA
    dq k0_ippsGFpECSignDSA
    dq k1_ippsGFpECSignDSA

segment .text
global ippsGFpECSignDSA:function (ippsGFpECSignDSA.LEndippsGFpECSignDSA - ippsGFpECSignDSA)
.Lin_ippsGFpECSignDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSignDSA:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSignDSA]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSignDSA:
