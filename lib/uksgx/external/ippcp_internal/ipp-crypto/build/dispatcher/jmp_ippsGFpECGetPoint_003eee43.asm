%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECGetPoint%+elf_symbol_type
extern n8_ippsGFpECGetPoint%+elf_symbol_type
extern y8_ippsGFpECGetPoint%+elf_symbol_type
extern e9_ippsGFpECGetPoint%+elf_symbol_type
extern l9_ippsGFpECGetPoint%+elf_symbol_type
extern n0_ippsGFpECGetPoint%+elf_symbol_type
extern k0_ippsGFpECGetPoint%+elf_symbol_type
extern k1_ippsGFpECGetPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECGetPoint
.Larraddr_ippsGFpECGetPoint:
    dq m7_ippsGFpECGetPoint
    dq n8_ippsGFpECGetPoint
    dq y8_ippsGFpECGetPoint
    dq e9_ippsGFpECGetPoint
    dq l9_ippsGFpECGetPoint
    dq n0_ippsGFpECGetPoint
    dq k0_ippsGFpECGetPoint
    dq k1_ippsGFpECGetPoint

segment .text
global ippsGFpECGetPoint:function (ippsGFpECGetPoint.LEndippsGFpECGetPoint - ippsGFpECGetPoint)
.Lin_ippsGFpECGetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECGetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECGetPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECGetPoint:
