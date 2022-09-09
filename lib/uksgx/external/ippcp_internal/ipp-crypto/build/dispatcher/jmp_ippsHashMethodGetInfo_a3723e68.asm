%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashMethodGetInfo%+elf_symbol_type
extern n8_ippsHashMethodGetInfo%+elf_symbol_type
extern y8_ippsHashMethodGetInfo%+elf_symbol_type
extern e9_ippsHashMethodGetInfo%+elf_symbol_type
extern l9_ippsHashMethodGetInfo%+elf_symbol_type
extern n0_ippsHashMethodGetInfo%+elf_symbol_type
extern k0_ippsHashMethodGetInfo%+elf_symbol_type
extern k1_ippsHashMethodGetInfo%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashMethodGetInfo
.Larraddr_ippsHashMethodGetInfo:
    dq m7_ippsHashMethodGetInfo
    dq n8_ippsHashMethodGetInfo
    dq y8_ippsHashMethodGetInfo
    dq e9_ippsHashMethodGetInfo
    dq l9_ippsHashMethodGetInfo
    dq n0_ippsHashMethodGetInfo
    dq k0_ippsHashMethodGetInfo
    dq k1_ippsHashMethodGetInfo

segment .text
global ippsHashMethodGetInfo:function (ippsHashMethodGetInfo.LEndippsHashMethodGetInfo - ippsHashMethodGetInfo)
.Lin_ippsHashMethodGetInfo:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashMethodGetInfo:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashMethodGetInfo]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashMethodGetInfo:
