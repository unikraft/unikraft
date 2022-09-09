%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpElementInit%+elf_symbol_type
extern n8_ippsGFpElementInit%+elf_symbol_type
extern y8_ippsGFpElementInit%+elf_symbol_type
extern e9_ippsGFpElementInit%+elf_symbol_type
extern l9_ippsGFpElementInit%+elf_symbol_type
extern n0_ippsGFpElementInit%+elf_symbol_type
extern k0_ippsGFpElementInit%+elf_symbol_type
extern k1_ippsGFpElementInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpElementInit
.Larraddr_ippsGFpElementInit:
    dq m7_ippsGFpElementInit
    dq n8_ippsGFpElementInit
    dq y8_ippsGFpElementInit
    dq e9_ippsGFpElementInit
    dq l9_ippsGFpElementInit
    dq n0_ippsGFpElementInit
    dq k0_ippsGFpElementInit
    dq k1_ippsGFpElementInit

segment .text
global ippsGFpElementInit:function (ippsGFpElementInit.LEndippsGFpElementInit - ippsGFpElementInit)
.Lin_ippsGFpElementInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpElementInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpElementInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpElementInit:
