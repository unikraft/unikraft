%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern n8_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern y8_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern e9_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern l9_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern n0_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern k0_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern k1_ippsGFpECSetPointHashBackCompatible_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetPointHashBackCompatible_rmf
.Larraddr_ippsGFpECSetPointHashBackCompatible_rmf:
    dq m7_ippsGFpECSetPointHashBackCompatible_rmf
    dq n8_ippsGFpECSetPointHashBackCompatible_rmf
    dq y8_ippsGFpECSetPointHashBackCompatible_rmf
    dq e9_ippsGFpECSetPointHashBackCompatible_rmf
    dq l9_ippsGFpECSetPointHashBackCompatible_rmf
    dq n0_ippsGFpECSetPointHashBackCompatible_rmf
    dq k0_ippsGFpECSetPointHashBackCompatible_rmf
    dq k1_ippsGFpECSetPointHashBackCompatible_rmf

segment .text
global ippsGFpECSetPointHashBackCompatible_rmf:function (ippsGFpECSetPointHashBackCompatible_rmf.LEndippsGFpECSetPointHashBackCompatible_rmf - ippsGFpECSetPointHashBackCompatible_rmf)
.Lin_ippsGFpECSetPointHashBackCompatible_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetPointHashBackCompatible_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetPointHashBackCompatible_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetPointHashBackCompatible_rmf:
