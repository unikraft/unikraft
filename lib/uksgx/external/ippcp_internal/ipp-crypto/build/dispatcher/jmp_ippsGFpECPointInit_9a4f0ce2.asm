%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECPointInit%+elf_symbol_type
extern n8_ippsGFpECPointInit%+elf_symbol_type
extern y8_ippsGFpECPointInit%+elf_symbol_type
extern e9_ippsGFpECPointInit%+elf_symbol_type
extern l9_ippsGFpECPointInit%+elf_symbol_type
extern n0_ippsGFpECPointInit%+elf_symbol_type
extern k0_ippsGFpECPointInit%+elf_symbol_type
extern k1_ippsGFpECPointInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECPointInit
.Larraddr_ippsGFpECPointInit:
    dq m7_ippsGFpECPointInit
    dq n8_ippsGFpECPointInit
    dq y8_ippsGFpECPointInit
    dq e9_ippsGFpECPointInit
    dq l9_ippsGFpECPointInit
    dq n0_ippsGFpECPointInit
    dq k0_ippsGFpECPointInit
    dq k1_ippsGFpECPointInit

segment .text
global ippsGFpECPointInit:function (ippsGFpECPointInit.LEndippsGFpECPointInit - ippsGFpECPointInit)
.Lin_ippsGFpECPointInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECPointInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECPointInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECPointInit:
