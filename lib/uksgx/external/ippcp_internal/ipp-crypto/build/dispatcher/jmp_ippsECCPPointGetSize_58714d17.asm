%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPPointGetSize%+elf_symbol_type
extern n8_ippsECCPPointGetSize%+elf_symbol_type
extern y8_ippsECCPPointGetSize%+elf_symbol_type
extern e9_ippsECCPPointGetSize%+elf_symbol_type
extern l9_ippsECCPPointGetSize%+elf_symbol_type
extern n0_ippsECCPPointGetSize%+elf_symbol_type
extern k0_ippsECCPPointGetSize%+elf_symbol_type
extern k1_ippsECCPPointGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPPointGetSize
.Larraddr_ippsECCPPointGetSize:
    dq m7_ippsECCPPointGetSize
    dq n8_ippsECCPPointGetSize
    dq y8_ippsECCPPointGetSize
    dq e9_ippsECCPPointGetSize
    dq l9_ippsECCPPointGetSize
    dq n0_ippsECCPPointGetSize
    dq k0_ippsECCPPointGetSize
    dq k1_ippsECCPPointGetSize

segment .text
global ippsECCPPointGetSize:function (ippsECCPPointGetSize.LEndippsECCPPointGetSize - ippsECCPPointGetSize)
.Lin_ippsECCPPointGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPPointGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPPointGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPPointGetSize:
