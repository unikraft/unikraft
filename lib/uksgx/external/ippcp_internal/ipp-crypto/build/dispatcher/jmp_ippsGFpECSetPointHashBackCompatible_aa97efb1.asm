%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern n8_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern y8_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern e9_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern l9_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern n0_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern k0_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern k1_ippsGFpECSetPointHashBackCompatible%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetPointHashBackCompatible
.Larraddr_ippsGFpECSetPointHashBackCompatible:
    dq m7_ippsGFpECSetPointHashBackCompatible
    dq n8_ippsGFpECSetPointHashBackCompatible
    dq y8_ippsGFpECSetPointHashBackCompatible
    dq e9_ippsGFpECSetPointHashBackCompatible
    dq l9_ippsGFpECSetPointHashBackCompatible
    dq n0_ippsGFpECSetPointHashBackCompatible
    dq k0_ippsGFpECSetPointHashBackCompatible
    dq k1_ippsGFpECSetPointHashBackCompatible

segment .text
global ippsGFpECSetPointHashBackCompatible:function (ippsGFpECSetPointHashBackCompatible.LEndippsGFpECSetPointHashBackCompatible - ippsGFpECSetPointHashBackCompatible)
.Lin_ippsGFpECSetPointHashBackCompatible:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetPointHashBackCompatible:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetPointHashBackCompatible]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetPointHashBackCompatible:
