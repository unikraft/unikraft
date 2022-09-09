%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECESStart_SM2%+elf_symbol_type
extern n8_ippsGFpECESStart_SM2%+elf_symbol_type
extern y8_ippsGFpECESStart_SM2%+elf_symbol_type
extern e9_ippsGFpECESStart_SM2%+elf_symbol_type
extern l9_ippsGFpECESStart_SM2%+elf_symbol_type
extern n0_ippsGFpECESStart_SM2%+elf_symbol_type
extern k0_ippsGFpECESStart_SM2%+elf_symbol_type
extern k1_ippsGFpECESStart_SM2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECESStart_SM2
.Larraddr_ippsGFpECESStart_SM2:
    dq m7_ippsGFpECESStart_SM2
    dq n8_ippsGFpECESStart_SM2
    dq y8_ippsGFpECESStart_SM2
    dq e9_ippsGFpECESStart_SM2
    dq l9_ippsGFpECESStart_SM2
    dq n0_ippsGFpECESStart_SM2
    dq k0_ippsGFpECESStart_SM2
    dq k1_ippsGFpECESStart_SM2

segment .text
global ippsGFpECESStart_SM2:function (ippsGFpECESStart_SM2.LEndippsGFpECESStart_SM2 - ippsGFpECESStart_SM2)
.Lin_ippsGFpECESStart_SM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECESStart_SM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECESStart_SM2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECESStart_SM2:
