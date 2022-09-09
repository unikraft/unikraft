%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECCmpPoint%+elf_symbol_type
extern n8_ippsGFpECCmpPoint%+elf_symbol_type
extern y8_ippsGFpECCmpPoint%+elf_symbol_type
extern e9_ippsGFpECCmpPoint%+elf_symbol_type
extern l9_ippsGFpECCmpPoint%+elf_symbol_type
extern n0_ippsGFpECCmpPoint%+elf_symbol_type
extern k0_ippsGFpECCmpPoint%+elf_symbol_type
extern k1_ippsGFpECCmpPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECCmpPoint
.Larraddr_ippsGFpECCmpPoint:
    dq m7_ippsGFpECCmpPoint
    dq n8_ippsGFpECCmpPoint
    dq y8_ippsGFpECCmpPoint
    dq e9_ippsGFpECCmpPoint
    dq l9_ippsGFpECCmpPoint
    dq n0_ippsGFpECCmpPoint
    dq k0_ippsGFpECCmpPoint
    dq k1_ippsGFpECCmpPoint

segment .text
global ippsGFpECCmpPoint:function (ippsGFpECCmpPoint.LEndippsGFpECCmpPoint - ippsGFpECCmpPoint)
.Lin_ippsGFpECCmpPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECCmpPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECCmpPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECCmpPoint:
