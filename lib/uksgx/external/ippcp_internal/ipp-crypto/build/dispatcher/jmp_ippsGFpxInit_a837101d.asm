%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpxInit%+elf_symbol_type
extern n8_ippsGFpxInit%+elf_symbol_type
extern y8_ippsGFpxInit%+elf_symbol_type
extern e9_ippsGFpxInit%+elf_symbol_type
extern l9_ippsGFpxInit%+elf_symbol_type
extern n0_ippsGFpxInit%+elf_symbol_type
extern k0_ippsGFpxInit%+elf_symbol_type
extern k1_ippsGFpxInit%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpxInit
.Larraddr_ippsGFpxInit:
    dq m7_ippsGFpxInit
    dq n8_ippsGFpxInit
    dq y8_ippsGFpxInit
    dq e9_ippsGFpxInit
    dq l9_ippsGFpxInit
    dq n0_ippsGFpxInit
    dq k0_ippsGFpxInit
    dq k1_ippsGFpxInit

segment .text
global ippsGFpxInit:function (ippsGFpxInit.LEndippsGFpxInit - ippsGFpxInit)
.Lin_ippsGFpxInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpxInit:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpxInit]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpxInit:
