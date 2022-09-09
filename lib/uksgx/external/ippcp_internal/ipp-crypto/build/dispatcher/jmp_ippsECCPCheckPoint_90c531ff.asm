%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPCheckPoint%+elf_symbol_type
extern n8_ippsECCPCheckPoint%+elf_symbol_type
extern y8_ippsECCPCheckPoint%+elf_symbol_type
extern e9_ippsECCPCheckPoint%+elf_symbol_type
extern l9_ippsECCPCheckPoint%+elf_symbol_type
extern n0_ippsECCPCheckPoint%+elf_symbol_type
extern k0_ippsECCPCheckPoint%+elf_symbol_type
extern k1_ippsECCPCheckPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPCheckPoint
.Larraddr_ippsECCPCheckPoint:
    dq m7_ippsECCPCheckPoint
    dq n8_ippsECCPCheckPoint
    dq y8_ippsECCPCheckPoint
    dq e9_ippsECCPCheckPoint
    dq l9_ippsECCPCheckPoint
    dq n0_ippsECCPCheckPoint
    dq k0_ippsECCPCheckPoint
    dq k1_ippsECCPCheckPoint

segment .text
global ippsECCPCheckPoint:function (ippsECCPCheckPoint.LEndippsECCPCheckPoint - ippsECCPCheckPoint)
.Lin_ippsECCPCheckPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPCheckPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPCheckPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPCheckPoint:
