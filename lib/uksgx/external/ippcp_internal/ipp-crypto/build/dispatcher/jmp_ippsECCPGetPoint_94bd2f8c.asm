%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPGetPoint%+elf_symbol_type
extern n8_ippsECCPGetPoint%+elf_symbol_type
extern y8_ippsECCPGetPoint%+elf_symbol_type
extern e9_ippsECCPGetPoint%+elf_symbol_type
extern l9_ippsECCPGetPoint%+elf_symbol_type
extern n0_ippsECCPGetPoint%+elf_symbol_type
extern k0_ippsECCPGetPoint%+elf_symbol_type
extern k1_ippsECCPGetPoint%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPGetPoint
.Larraddr_ippsECCPGetPoint:
    dq m7_ippsECCPGetPoint
    dq n8_ippsECCPGetPoint
    dq y8_ippsECCPGetPoint
    dq e9_ippsECCPGetPoint
    dq l9_ippsECCPGetPoint
    dq n0_ippsECCPGetPoint
    dq k0_ippsECCPGetPoint
    dq k1_ippsECCPGetPoint

segment .text
global ippsECCPGetPoint:function (ippsECCPGetPoint.LEndippsECCPGetPoint - ippsECCPGetPoint)
.Lin_ippsECCPGetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPGetPoint:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPGetPoint]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPGetPoint:
