%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMACDuplicate_rmf%+elf_symbol_type
extern n8_ippsHMACDuplicate_rmf%+elf_symbol_type
extern y8_ippsHMACDuplicate_rmf%+elf_symbol_type
extern e9_ippsHMACDuplicate_rmf%+elf_symbol_type
extern l9_ippsHMACDuplicate_rmf%+elf_symbol_type
extern n0_ippsHMACDuplicate_rmf%+elf_symbol_type
extern k0_ippsHMACDuplicate_rmf%+elf_symbol_type
extern k1_ippsHMACDuplicate_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMACDuplicate_rmf
.Larraddr_ippsHMACDuplicate_rmf:
    dq m7_ippsHMACDuplicate_rmf
    dq n8_ippsHMACDuplicate_rmf
    dq y8_ippsHMACDuplicate_rmf
    dq e9_ippsHMACDuplicate_rmf
    dq l9_ippsHMACDuplicate_rmf
    dq n0_ippsHMACDuplicate_rmf
    dq k0_ippsHMACDuplicate_rmf
    dq k1_ippsHMACDuplicate_rmf

segment .text
global ippsHMACDuplicate_rmf:function (ippsHMACDuplicate_rmf.LEndippsHMACDuplicate_rmf - ippsHMACDuplicate_rmf)
.Lin_ippsHMACDuplicate_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMACDuplicate_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMACDuplicate_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMACDuplicate_rmf:
