%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECAddPoint%+elf_symbol_type
extern n8_ippsGFpECAddPoint%+elf_symbol_type
extern y8_ippsGFpECAddPoint%+elf_symbol_type
extern e9_ippsGFpECAddPoint%+elf_symbol_type
extern l9_ippsGFpECAddPoint%+elf_symbol_type
extern n0_ippsGFpECAddPoint%+elf_symbol_type
extern k0_ippsGFpECAddPoint%+elf_symbol_type
extern k1_ippsGFpECAddPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECAddPoint
.Larraddr_ippsGFpECAddPoint:
    dq m7_ippsGFpECAddPoint
    dq n8_ippsGFpECAddPoint
    dq y8_ippsGFpECAddPoint
    dq e9_ippsGFpECAddPoint
    dq l9_ippsGFpECAddPoint
    dq n0_ippsGFpECAddPoint
    dq k0_ippsGFpECAddPoint
    dq k1_ippsGFpECAddPoint

segment .text
global ippsGFpECAddPoint:function (ippsGFpECAddPoint.LEndippsGFpECAddPoint - ippsGFpECAddPoint)
.Lin_ippsGFpECAddPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECAddPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECAddPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECAddPoint:
