%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPGetSize%+elf_symbol_type
extern n8_ippsECCPGetSize%+elf_symbol_type
extern y8_ippsECCPGetSize%+elf_symbol_type
extern e9_ippsECCPGetSize%+elf_symbol_type
extern l9_ippsECCPGetSize%+elf_symbol_type
extern n0_ippsECCPGetSize%+elf_symbol_type
extern k0_ippsECCPGetSize%+elf_symbol_type
extern k1_ippsECCPGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPGetSize
.Larraddr_ippsECCPGetSize:
    dq m7_ippsECCPGetSize
    dq n8_ippsECCPGetSize
    dq y8_ippsECCPGetSize
    dq e9_ippsECCPGetSize
    dq l9_ippsECCPGetSize
    dq n0_ippsECCPGetSize
    dq k0_ippsECCPGetSize
    dq k1_ippsECCPGetSize

segment .text
global ippsECCPGetSize:function (ippsECCPGetSize.LEndippsECCPGetSize - ippsECCPGetSize)
.Lin_ippsECCPGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPGetSize:
