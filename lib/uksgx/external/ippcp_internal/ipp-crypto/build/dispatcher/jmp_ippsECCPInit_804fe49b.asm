%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPInit%+elf_symbol_type
extern n8_ippsECCPInit%+elf_symbol_type
extern y8_ippsECCPInit%+elf_symbol_type
extern e9_ippsECCPInit%+elf_symbol_type
extern l9_ippsECCPInit%+elf_symbol_type
extern n0_ippsECCPInit%+elf_symbol_type
extern k0_ippsECCPInit%+elf_symbol_type
extern k1_ippsECCPInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPInit
.Larraddr_ippsECCPInit:
    dq m7_ippsECCPInit
    dq n8_ippsECCPInit
    dq y8_ippsECCPInit
    dq e9_ippsECCPInit
    dq l9_ippsECCPInit
    dq n0_ippsECCPInit
    dq k0_ippsECCPInit
    dq k1_ippsECCPInit

segment .text
global ippsECCPInit:function (ippsECCPInit.LEndippsECCPInit - ippsECCPInit)
.Lin_ippsECCPInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPInit:
