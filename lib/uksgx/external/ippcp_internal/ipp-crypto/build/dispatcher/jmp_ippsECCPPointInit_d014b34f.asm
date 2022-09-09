%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPPointInit%+elf_symbol_type
extern n8_ippsECCPPointInit%+elf_symbol_type
extern y8_ippsECCPPointInit%+elf_symbol_type
extern e9_ippsECCPPointInit%+elf_symbol_type
extern l9_ippsECCPPointInit%+elf_symbol_type
extern n0_ippsECCPPointInit%+elf_symbol_type
extern k0_ippsECCPPointInit%+elf_symbol_type
extern k1_ippsECCPPointInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPPointInit
.Larraddr_ippsECCPPointInit:
    dq m7_ippsECCPPointInit
    dq n8_ippsECCPPointInit
    dq y8_ippsECCPPointInit
    dq e9_ippsECCPPointInit
    dq l9_ippsECCPPointInit
    dq n0_ippsECCPPointInit
    dq k0_ippsECCPPointInit
    dq k1_ippsECCPPointInit

segment .text
global ippsECCPPointInit:function (ippsECCPPointInit.LEndippsECCPPointInit - ippsECCPPointInit)
.Lin_ippsECCPPointInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPPointInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPPointInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPPointInit:
