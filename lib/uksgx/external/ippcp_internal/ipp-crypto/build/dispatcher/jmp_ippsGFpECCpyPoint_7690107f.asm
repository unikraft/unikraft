%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECCpyPoint%+elf_symbol_type
extern n8_ippsGFpECCpyPoint%+elf_symbol_type
extern y8_ippsGFpECCpyPoint%+elf_symbol_type
extern e9_ippsGFpECCpyPoint%+elf_symbol_type
extern l9_ippsGFpECCpyPoint%+elf_symbol_type
extern n0_ippsGFpECCpyPoint%+elf_symbol_type
extern k0_ippsGFpECCpyPoint%+elf_symbol_type
extern k1_ippsGFpECCpyPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECCpyPoint
.Larraddr_ippsGFpECCpyPoint:
    dq m7_ippsGFpECCpyPoint
    dq n8_ippsGFpECCpyPoint
    dq y8_ippsGFpECCpyPoint
    dq e9_ippsGFpECCpyPoint
    dq l9_ippsGFpECCpyPoint
    dq n0_ippsGFpECCpyPoint
    dq k0_ippsGFpECCpyPoint
    dq k1_ippsGFpECCpyPoint

segment .text
global ippsGFpECCpyPoint:function (ippsGFpECCpyPoint.LEndippsGFpECCpyPoint - ippsGFpECCpyPoint)
.Lin_ippsGFpECCpyPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECCpyPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECCpyPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECCpyPoint:
