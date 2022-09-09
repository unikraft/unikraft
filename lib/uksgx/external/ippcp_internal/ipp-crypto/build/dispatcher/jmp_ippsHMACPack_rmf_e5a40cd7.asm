%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMACPack_rmf%+elf_symbol_type
extern n8_ippsHMACPack_rmf%+elf_symbol_type
extern y8_ippsHMACPack_rmf%+elf_symbol_type
extern e9_ippsHMACPack_rmf%+elf_symbol_type
extern l9_ippsHMACPack_rmf%+elf_symbol_type
extern n0_ippsHMACPack_rmf%+elf_symbol_type
extern k0_ippsHMACPack_rmf%+elf_symbol_type
extern k1_ippsHMACPack_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMACPack_rmf
.Larraddr_ippsHMACPack_rmf:
    dq m7_ippsHMACPack_rmf
    dq n8_ippsHMACPack_rmf
    dq y8_ippsHMACPack_rmf
    dq e9_ippsHMACPack_rmf
    dq l9_ippsHMACPack_rmf
    dq n0_ippsHMACPack_rmf
    dq k0_ippsHMACPack_rmf
    dq k1_ippsHMACPack_rmf

segment .text
global ippsHMACPack_rmf:function (ippsHMACPack_rmf.LEndippsHMACPack_rmf - ippsHMACPack_rmf)
.Lin_ippsHMACPack_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMACPack_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMACPack_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMACPack_rmf:
