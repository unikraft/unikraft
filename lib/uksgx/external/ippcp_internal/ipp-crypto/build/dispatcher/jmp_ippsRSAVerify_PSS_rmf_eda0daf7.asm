%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern n8_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern y8_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern e9_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern l9_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern n0_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern k0_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern k1_ippsRSAVerify_PSS_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSAVerify_PSS_rmf
.Larraddr_ippsRSAVerify_PSS_rmf:
    dq m7_ippsRSAVerify_PSS_rmf
    dq n8_ippsRSAVerify_PSS_rmf
    dq y8_ippsRSAVerify_PSS_rmf
    dq e9_ippsRSAVerify_PSS_rmf
    dq l9_ippsRSAVerify_PSS_rmf
    dq n0_ippsRSAVerify_PSS_rmf
    dq k0_ippsRSAVerify_PSS_rmf
    dq k1_ippsRSAVerify_PSS_rmf

segment .text
global ippsRSAVerify_PSS_rmf:function (ippsRSAVerify_PSS_rmf.LEndippsRSAVerify_PSS_rmf - ippsRSAVerify_PSS_rmf)
.Lin_ippsRSAVerify_PSS_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSAVerify_PSS_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSAVerify_PSS_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSAVerify_PSS_rmf:
