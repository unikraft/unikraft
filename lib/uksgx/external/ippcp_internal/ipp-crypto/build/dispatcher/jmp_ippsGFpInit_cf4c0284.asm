%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpInit%+elf_symbol_type
extern n8_ippsGFpInit%+elf_symbol_type
extern y8_ippsGFpInit%+elf_symbol_type
extern e9_ippsGFpInit%+elf_symbol_type
extern l9_ippsGFpInit%+elf_symbol_type
extern n0_ippsGFpInit%+elf_symbol_type
extern k0_ippsGFpInit%+elf_symbol_type
extern k1_ippsGFpInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpInit
.Larraddr_ippsGFpInit:
    dq m7_ippsGFpInit
    dq n8_ippsGFpInit
    dq y8_ippsGFpInit
    dq e9_ippsGFpInit
    dq l9_ippsGFpInit
    dq n0_ippsGFpInit
    dq k0_ippsGFpInit
    dq k1_ippsGFpInit

segment .text
global ippsGFpInit:function (ippsGFpInit.LEndippsGFpInit - ippsGFpInit)
.Lin_ippsGFpInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpInit:
