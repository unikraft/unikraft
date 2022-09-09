%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSAVerify_PSS%+elf_symbol_type
extern n8_ippsRSAVerify_PSS%+elf_symbol_type
extern y8_ippsRSAVerify_PSS%+elf_symbol_type
extern e9_ippsRSAVerify_PSS%+elf_symbol_type
extern l9_ippsRSAVerify_PSS%+elf_symbol_type
extern n0_ippsRSAVerify_PSS%+elf_symbol_type
extern k0_ippsRSAVerify_PSS%+elf_symbol_type
extern k1_ippsRSAVerify_PSS%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSAVerify_PSS
.Larraddr_ippsRSAVerify_PSS:
    dq m7_ippsRSAVerify_PSS
    dq n8_ippsRSAVerify_PSS
    dq y8_ippsRSAVerify_PSS
    dq e9_ippsRSAVerify_PSS
    dq l9_ippsRSAVerify_PSS
    dq n0_ippsRSAVerify_PSS
    dq k0_ippsRSAVerify_PSS
    dq k1_ippsRSAVerify_PSS

segment .text
global ippsRSAVerify_PSS:function (ippsRSAVerify_PSS.LEndippsRSAVerify_PSS - ippsRSAVerify_PSS)
.Lin_ippsRSAVerify_PSS:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSAVerify_PSS:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSAVerify_PSS]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSAVerify_PSS:
