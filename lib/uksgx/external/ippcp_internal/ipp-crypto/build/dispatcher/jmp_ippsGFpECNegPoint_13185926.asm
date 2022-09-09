%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECNegPoint%+elf_symbol_type
extern n8_ippsGFpECNegPoint%+elf_symbol_type
extern y8_ippsGFpECNegPoint%+elf_symbol_type
extern e9_ippsGFpECNegPoint%+elf_symbol_type
extern l9_ippsGFpECNegPoint%+elf_symbol_type
extern n0_ippsGFpECNegPoint%+elf_symbol_type
extern k0_ippsGFpECNegPoint%+elf_symbol_type
extern k1_ippsGFpECNegPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECNegPoint
.Larraddr_ippsGFpECNegPoint:
    dq m7_ippsGFpECNegPoint
    dq n8_ippsGFpECNegPoint
    dq y8_ippsGFpECNegPoint
    dq e9_ippsGFpECNegPoint
    dq l9_ippsGFpECNegPoint
    dq n0_ippsGFpECNegPoint
    dq k0_ippsGFpECNegPoint
    dq k1_ippsGFpECNegPoint

segment .text
global ippsGFpECNegPoint:function (ippsGFpECNegPoint.LEndippsGFpECNegPoint - ippsGFpECNegPoint)
.Lin_ippsGFpECNegPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECNegPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECNegPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECNegPoint:
