%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPSetStd%+elf_symbol_type
extern n8_ippsECCPSetStd%+elf_symbol_type
extern y8_ippsECCPSetStd%+elf_symbol_type
extern e9_ippsECCPSetStd%+elf_symbol_type
extern l9_ippsECCPSetStd%+elf_symbol_type
extern n0_ippsECCPSetStd%+elf_symbol_type
extern k0_ippsECCPSetStd%+elf_symbol_type
extern k1_ippsECCPSetStd%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPSetStd
.Larraddr_ippsECCPSetStd:
    dq m7_ippsECCPSetStd
    dq n8_ippsECCPSetStd
    dq y8_ippsECCPSetStd
    dq e9_ippsECCPSetStd
    dq l9_ippsECCPSetStd
    dq n0_ippsECCPSetStd
    dq k0_ippsECCPSetStd
    dq k1_ippsECCPSetStd

segment .text
global ippsECCPSetStd:function (ippsECCPSetStd.LEndippsECCPSetStd - ippsECCPSetStd)
.Lin_ippsECCPSetStd:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPSetStd:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPSetStd]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPSetStd:
