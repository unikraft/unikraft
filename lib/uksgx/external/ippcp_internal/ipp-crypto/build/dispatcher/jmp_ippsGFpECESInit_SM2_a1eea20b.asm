%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECESInit_SM2%+elf_symbol_type
extern n8_ippsGFpECESInit_SM2%+elf_symbol_type
extern y8_ippsGFpECESInit_SM2%+elf_symbol_type
extern e9_ippsGFpECESInit_SM2%+elf_symbol_type
extern l9_ippsGFpECESInit_SM2%+elf_symbol_type
extern n0_ippsGFpECESInit_SM2%+elf_symbol_type
extern k0_ippsGFpECESInit_SM2%+elf_symbol_type
extern k1_ippsGFpECESInit_SM2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECESInit_SM2
.Larraddr_ippsGFpECESInit_SM2:
    dq m7_ippsGFpECESInit_SM2
    dq n8_ippsGFpECESInit_SM2
    dq y8_ippsGFpECESInit_SM2
    dq e9_ippsGFpECESInit_SM2
    dq l9_ippsGFpECESInit_SM2
    dq n0_ippsGFpECESInit_SM2
    dq k0_ippsGFpECESInit_SM2
    dq k1_ippsGFpECESInit_SM2

segment .text
global ippsGFpECESInit_SM2:function (ippsGFpECESInit_SM2.LEndippsGFpECESInit_SM2 - ippsGFpECESInit_SM2)
.Lin_ippsGFpECESInit_SM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECESInit_SM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECESInit_SM2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECESInit_SM2:
