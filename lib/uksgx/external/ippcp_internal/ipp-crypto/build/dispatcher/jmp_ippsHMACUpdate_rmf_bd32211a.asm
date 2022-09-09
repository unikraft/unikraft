%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMACUpdate_rmf%+elf_symbol_type
extern n8_ippsHMACUpdate_rmf%+elf_symbol_type
extern y8_ippsHMACUpdate_rmf%+elf_symbol_type
extern e9_ippsHMACUpdate_rmf%+elf_symbol_type
extern l9_ippsHMACUpdate_rmf%+elf_symbol_type
extern n0_ippsHMACUpdate_rmf%+elf_symbol_type
extern k0_ippsHMACUpdate_rmf%+elf_symbol_type
extern k1_ippsHMACUpdate_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMACUpdate_rmf
.Larraddr_ippsHMACUpdate_rmf:
    dq m7_ippsHMACUpdate_rmf
    dq n8_ippsHMACUpdate_rmf
    dq y8_ippsHMACUpdate_rmf
    dq e9_ippsHMACUpdate_rmf
    dq l9_ippsHMACUpdate_rmf
    dq n0_ippsHMACUpdate_rmf
    dq k0_ippsHMACUpdate_rmf
    dq k1_ippsHMACUpdate_rmf

segment .text
global ippsHMACUpdate_rmf:function (ippsHMACUpdate_rmf.LEndippsHMACUpdate_rmf - ippsHMACUpdate_rmf)
.Lin_ippsHMACUpdate_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMACUpdate_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMACUpdate_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMACUpdate_rmf:
