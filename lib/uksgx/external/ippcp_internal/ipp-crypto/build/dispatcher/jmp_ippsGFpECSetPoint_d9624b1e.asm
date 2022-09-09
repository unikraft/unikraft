%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetPoint%+elf_symbol_type
extern n8_ippsGFpECSetPoint%+elf_symbol_type
extern y8_ippsGFpECSetPoint%+elf_symbol_type
extern e9_ippsGFpECSetPoint%+elf_symbol_type
extern l9_ippsGFpECSetPoint%+elf_symbol_type
extern n0_ippsGFpECSetPoint%+elf_symbol_type
extern k0_ippsGFpECSetPoint%+elf_symbol_type
extern k1_ippsGFpECSetPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetPoint
.Larraddr_ippsGFpECSetPoint:
    dq m7_ippsGFpECSetPoint
    dq n8_ippsGFpECSetPoint
    dq y8_ippsGFpECSetPoint
    dq e9_ippsGFpECSetPoint
    dq l9_ippsGFpECSetPoint
    dq n0_ippsGFpECSetPoint
    dq k0_ippsGFpECSetPoint
    dq k1_ippsGFpECSetPoint

segment .text
global ippsGFpECSetPoint:function (ippsGFpECSetPoint.LEndippsGFpECSetPoint - ippsGFpECSetPoint)
.Lin_ippsGFpECSetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetPoint:
