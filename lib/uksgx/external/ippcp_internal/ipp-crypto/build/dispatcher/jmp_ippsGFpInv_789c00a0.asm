%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpInv%+elf_symbol_type
extern n8_ippsGFpInv%+elf_symbol_type
extern y8_ippsGFpInv%+elf_symbol_type
extern e9_ippsGFpInv%+elf_symbol_type
extern l9_ippsGFpInv%+elf_symbol_type
extern n0_ippsGFpInv%+elf_symbol_type
extern k0_ippsGFpInv%+elf_symbol_type
extern k1_ippsGFpInv%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpInv
.Larraddr_ippsGFpInv:
    dq m7_ippsGFpInv
    dq n8_ippsGFpInv
    dq y8_ippsGFpInv
    dq e9_ippsGFpInv
    dq l9_ippsGFpInv
    dq n0_ippsGFpInv
    dq k0_ippsGFpInv
    dq k1_ippsGFpInv

segment .text
global ippsGFpInv:function (ippsGFpInv.LEndippsGFpInv - ippsGFpInv)
.Lin_ippsGFpInv:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpInv:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpInv]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpInv:
