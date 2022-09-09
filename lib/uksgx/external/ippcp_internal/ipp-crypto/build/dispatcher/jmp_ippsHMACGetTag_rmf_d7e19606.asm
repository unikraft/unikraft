%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMACGetTag_rmf%+elf_symbol_type
extern n8_ippsHMACGetTag_rmf%+elf_symbol_type
extern y8_ippsHMACGetTag_rmf%+elf_symbol_type
extern e9_ippsHMACGetTag_rmf%+elf_symbol_type
extern l9_ippsHMACGetTag_rmf%+elf_symbol_type
extern n0_ippsHMACGetTag_rmf%+elf_symbol_type
extern k0_ippsHMACGetTag_rmf%+elf_symbol_type
extern k1_ippsHMACGetTag_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMACGetTag_rmf
.Larraddr_ippsHMACGetTag_rmf:
    dq m7_ippsHMACGetTag_rmf
    dq n8_ippsHMACGetTag_rmf
    dq y8_ippsHMACGetTag_rmf
    dq e9_ippsHMACGetTag_rmf
    dq l9_ippsHMACGetTag_rmf
    dq n0_ippsHMACGetTag_rmf
    dq k0_ippsHMACGetTag_rmf
    dq k1_ippsHMACGetTag_rmf

segment .text
global ippsHMACGetTag_rmf:function (ippsHMACGetTag_rmf.LEndippsHMACGetTag_rmf - ippsHMACGetTag_rmf)
.Lin_ippsHMACGetTag_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMACGetTag_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMACGetTag_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMACGetTag_rmf:
