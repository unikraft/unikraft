%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPAddPoint%+elf_symbol_type
extern n8_ippsECCPAddPoint%+elf_symbol_type
extern y8_ippsECCPAddPoint%+elf_symbol_type
extern e9_ippsECCPAddPoint%+elf_symbol_type
extern l9_ippsECCPAddPoint%+elf_symbol_type
extern n0_ippsECCPAddPoint%+elf_symbol_type
extern k0_ippsECCPAddPoint%+elf_symbol_type
extern k1_ippsECCPAddPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPAddPoint
.Larraddr_ippsECCPAddPoint:
    dq m7_ippsECCPAddPoint
    dq n8_ippsECCPAddPoint
    dq y8_ippsECCPAddPoint
    dq e9_ippsECCPAddPoint
    dq l9_ippsECCPAddPoint
    dq n0_ippsECCPAddPoint
    dq k0_ippsECCPAddPoint
    dq k1_ippsECCPAddPoint

segment .text
global ippsECCPAddPoint:function (ippsECCPAddPoint.LEndippsECCPAddPoint - ippsECCPAddPoint)
.Lin_ippsECCPAddPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPAddPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPAddPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPAddPoint:
