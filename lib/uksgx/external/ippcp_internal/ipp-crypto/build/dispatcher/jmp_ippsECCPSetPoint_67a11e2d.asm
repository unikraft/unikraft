%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPSetPoint%+elf_symbol_type
extern n8_ippsECCPSetPoint%+elf_symbol_type
extern y8_ippsECCPSetPoint%+elf_symbol_type
extern e9_ippsECCPSetPoint%+elf_symbol_type
extern l9_ippsECCPSetPoint%+elf_symbol_type
extern n0_ippsECCPSetPoint%+elf_symbol_type
extern k0_ippsECCPSetPoint%+elf_symbol_type
extern k1_ippsECCPSetPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPSetPoint
.Larraddr_ippsECCPSetPoint:
    dq m7_ippsECCPSetPoint
    dq n8_ippsECCPSetPoint
    dq y8_ippsECCPSetPoint
    dq e9_ippsECCPSetPoint
    dq l9_ippsECCPSetPoint
    dq n0_ippsECCPSetPoint
    dq k0_ippsECCPSetPoint
    dq k1_ippsECCPSetPoint

segment .text
global ippsECCPSetPoint:function (ippsECCPSetPoint.LEndippsECCPSetPoint - ippsECCPSetPoint)
.Lin_ippsECCPSetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPSetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPSetPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPSetPoint:
