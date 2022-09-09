%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMACFinal_rmf%+elf_symbol_type
extern n8_ippsHMACFinal_rmf%+elf_symbol_type
extern y8_ippsHMACFinal_rmf%+elf_symbol_type
extern e9_ippsHMACFinal_rmf%+elf_symbol_type
extern l9_ippsHMACFinal_rmf%+elf_symbol_type
extern n0_ippsHMACFinal_rmf%+elf_symbol_type
extern k0_ippsHMACFinal_rmf%+elf_symbol_type
extern k1_ippsHMACFinal_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMACFinal_rmf
.Larraddr_ippsHMACFinal_rmf:
    dq m7_ippsHMACFinal_rmf
    dq n8_ippsHMACFinal_rmf
    dq y8_ippsHMACFinal_rmf
    dq e9_ippsHMACFinal_rmf
    dq l9_ippsHMACFinal_rmf
    dq n0_ippsHMACFinal_rmf
    dq k0_ippsHMACFinal_rmf
    dq k1_ippsHMACFinal_rmf

segment .text
global ippsHMACFinal_rmf:function (ippsHMACFinal_rmf.LEndippsHMACFinal_rmf - ippsHMACFinal_rmf)
.Lin_ippsHMACFinal_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMACFinal_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMACFinal_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMACFinal_rmf:
