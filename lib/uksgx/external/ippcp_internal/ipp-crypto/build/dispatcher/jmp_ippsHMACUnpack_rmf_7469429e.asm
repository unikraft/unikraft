%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMACUnpack_rmf%+elf_symbol_type
extern n8_ippsHMACUnpack_rmf%+elf_symbol_type
extern y8_ippsHMACUnpack_rmf%+elf_symbol_type
extern e9_ippsHMACUnpack_rmf%+elf_symbol_type
extern l9_ippsHMACUnpack_rmf%+elf_symbol_type
extern n0_ippsHMACUnpack_rmf%+elf_symbol_type
extern k0_ippsHMACUnpack_rmf%+elf_symbol_type
extern k1_ippsHMACUnpack_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMACUnpack_rmf
.Larraddr_ippsHMACUnpack_rmf:
    dq m7_ippsHMACUnpack_rmf
    dq n8_ippsHMACUnpack_rmf
    dq y8_ippsHMACUnpack_rmf
    dq e9_ippsHMACUnpack_rmf
    dq l9_ippsHMACUnpack_rmf
    dq n0_ippsHMACUnpack_rmf
    dq k0_ippsHMACUnpack_rmf
    dq k1_ippsHMACUnpack_rmf

segment .text
global ippsHMACUnpack_rmf:function (ippsHMACUnpack_rmf.LEndippsHMACUnpack_rmf - ippsHMACUnpack_rmf)
.Lin_ippsHMACUnpack_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMACUnpack_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMACUnpack_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMACUnpack_rmf:
