%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPNegativePoint%+elf_symbol_type
extern n8_ippsECCPNegativePoint%+elf_symbol_type
extern y8_ippsECCPNegativePoint%+elf_symbol_type
extern e9_ippsECCPNegativePoint%+elf_symbol_type
extern l9_ippsECCPNegativePoint%+elf_symbol_type
extern n0_ippsECCPNegativePoint%+elf_symbol_type
extern k0_ippsECCPNegativePoint%+elf_symbol_type
extern k1_ippsECCPNegativePoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPNegativePoint
.Larraddr_ippsECCPNegativePoint:
    dq m7_ippsECCPNegativePoint
    dq n8_ippsECCPNegativePoint
    dq y8_ippsECCPNegativePoint
    dq e9_ippsECCPNegativePoint
    dq l9_ippsECCPNegativePoint
    dq n0_ippsECCPNegativePoint
    dq k0_ippsECCPNegativePoint
    dq k1_ippsECCPNegativePoint

segment .text
global ippsECCPNegativePoint:function (ippsECCPNegativePoint.LEndippsECCPNegativePoint - ippsECCPNegativePoint)
.Lin_ippsECCPNegativePoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPNegativePoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPNegativePoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPNegativePoint:
