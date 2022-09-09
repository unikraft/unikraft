%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPComparePoint%+elf_symbol_type
extern n8_ippsECCPComparePoint%+elf_symbol_type
extern y8_ippsECCPComparePoint%+elf_symbol_type
extern e9_ippsECCPComparePoint%+elf_symbol_type
extern l9_ippsECCPComparePoint%+elf_symbol_type
extern n0_ippsECCPComparePoint%+elf_symbol_type
extern k0_ippsECCPComparePoint%+elf_symbol_type
extern k1_ippsECCPComparePoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPComparePoint
.Larraddr_ippsECCPComparePoint:
    dq m7_ippsECCPComparePoint
    dq n8_ippsECCPComparePoint
    dq y8_ippsECCPComparePoint
    dq e9_ippsECCPComparePoint
    dq l9_ippsECCPComparePoint
    dq n0_ippsECCPComparePoint
    dq k0_ippsECCPComparePoint
    dq k1_ippsECCPComparePoint

segment .text
global ippsECCPComparePoint:function (ippsECCPComparePoint.LEndippsECCPComparePoint - ippsECCPComparePoint)
.Lin_ippsECCPComparePoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPComparePoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPComparePoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPComparePoint:
