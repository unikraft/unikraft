%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpGetInfo%+elf_symbol_type
extern n8_ippsGFpGetInfo%+elf_symbol_type
extern y8_ippsGFpGetInfo%+elf_symbol_type
extern e9_ippsGFpGetInfo%+elf_symbol_type
extern l9_ippsGFpGetInfo%+elf_symbol_type
extern n0_ippsGFpGetInfo%+elf_symbol_type
extern k0_ippsGFpGetInfo%+elf_symbol_type
extern k1_ippsGFpGetInfo%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpGetInfo
.Larraddr_ippsGFpGetInfo:
    dq m7_ippsGFpGetInfo
    dq n8_ippsGFpGetInfo
    dq y8_ippsGFpGetInfo
    dq e9_ippsGFpGetInfo
    dq l9_ippsGFpGetInfo
    dq n0_ippsGFpGetInfo
    dq k0_ippsGFpGetInfo
    dq k1_ippsGFpGetInfo

segment .text
global ippsGFpGetInfo:function (ippsGFpGetInfo.LEndippsGFpGetInfo - ippsGFpGetInfo)
.Lin_ippsGFpGetInfo:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpGetInfo:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpGetInfo]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpGetInfo:
