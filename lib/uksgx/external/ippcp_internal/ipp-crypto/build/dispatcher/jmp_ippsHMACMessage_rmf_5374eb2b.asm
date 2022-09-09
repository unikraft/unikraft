%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMACMessage_rmf%+elf_symbol_type
extern n8_ippsHMACMessage_rmf%+elf_symbol_type
extern y8_ippsHMACMessage_rmf%+elf_symbol_type
extern e9_ippsHMACMessage_rmf%+elf_symbol_type
extern l9_ippsHMACMessage_rmf%+elf_symbol_type
extern n0_ippsHMACMessage_rmf%+elf_symbol_type
extern k0_ippsHMACMessage_rmf%+elf_symbol_type
extern k1_ippsHMACMessage_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMACMessage_rmf
.Larraddr_ippsHMACMessage_rmf:
    dq m7_ippsHMACMessage_rmf
    dq n8_ippsHMACMessage_rmf
    dq y8_ippsHMACMessage_rmf
    dq e9_ippsHMACMessage_rmf
    dq l9_ippsHMACMessage_rmf
    dq n0_ippsHMACMessage_rmf
    dq k0_ippsHMACMessage_rmf
    dq k1_ippsHMACMessage_rmf

segment .text
global ippsHMACMessage_rmf:function (ippsHMACMessage_rmf.LEndippsHMACMessage_rmf - ippsHMACMessage_rmf)
.Lin_ippsHMACMessage_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMACMessage_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMACMessage_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMACMessage_rmf:
