%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern n8_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern y8_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern e9_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern l9_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern n0_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern k0_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern k1_ippsECCPBindGxyTblStdSM2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPBindGxyTblStdSM2
.Larraddr_ippsECCPBindGxyTblStdSM2:
    dq m7_ippsECCPBindGxyTblStdSM2
    dq n8_ippsECCPBindGxyTblStdSM2
    dq y8_ippsECCPBindGxyTblStdSM2
    dq e9_ippsECCPBindGxyTblStdSM2
    dq l9_ippsECCPBindGxyTblStdSM2
    dq n0_ippsECCPBindGxyTblStdSM2
    dq k0_ippsECCPBindGxyTblStdSM2
    dq k1_ippsECCPBindGxyTblStdSM2

segment .text
global ippsECCPBindGxyTblStdSM2:function (ippsECCPBindGxyTblStdSM2.LEndippsECCPBindGxyTblStdSM2 - ippsECCPBindGxyTblStdSM2)
.Lin_ippsECCPBindGxyTblStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPBindGxyTblStdSM2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPBindGxyTblStdSM2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPBindGxyTblStdSM2:
