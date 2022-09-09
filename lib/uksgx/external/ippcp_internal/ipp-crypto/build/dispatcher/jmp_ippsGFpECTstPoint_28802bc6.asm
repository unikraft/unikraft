%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECTstPoint%+elf_symbol_type
extern n8_ippsGFpECTstPoint%+elf_symbol_type
extern y8_ippsGFpECTstPoint%+elf_symbol_type
extern e9_ippsGFpECTstPoint%+elf_symbol_type
extern l9_ippsGFpECTstPoint%+elf_symbol_type
extern n0_ippsGFpECTstPoint%+elf_symbol_type
extern k0_ippsGFpECTstPoint%+elf_symbol_type
extern k1_ippsGFpECTstPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECTstPoint
.Larraddr_ippsGFpECTstPoint:
    dq m7_ippsGFpECTstPoint
    dq n8_ippsGFpECTstPoint
    dq y8_ippsGFpECTstPoint
    dq e9_ippsGFpECTstPoint
    dq l9_ippsGFpECTstPoint
    dq n0_ippsGFpECTstPoint
    dq k0_ippsGFpECTstPoint
    dq k1_ippsGFpECTstPoint

segment .text
global ippsGFpECTstPoint:function (ippsGFpECTstPoint.LEndippsGFpECTstPoint - ippsGFpECTstPoint)
.Lin_ippsGFpECTstPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECTstPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECTstPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECTstPoint:
