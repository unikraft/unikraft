%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashDuplicate_rmf%+elf_symbol_type
extern n8_ippsHashDuplicate_rmf%+elf_symbol_type
extern y8_ippsHashDuplicate_rmf%+elf_symbol_type
extern e9_ippsHashDuplicate_rmf%+elf_symbol_type
extern l9_ippsHashDuplicate_rmf%+elf_symbol_type
extern n0_ippsHashDuplicate_rmf%+elf_symbol_type
extern k0_ippsHashDuplicate_rmf%+elf_symbol_type
extern k1_ippsHashDuplicate_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashDuplicate_rmf
.Larraddr_ippsHashDuplicate_rmf:
    dq m7_ippsHashDuplicate_rmf
    dq n8_ippsHashDuplicate_rmf
    dq y8_ippsHashDuplicate_rmf
    dq e9_ippsHashDuplicate_rmf
    dq l9_ippsHashDuplicate_rmf
    dq n0_ippsHashDuplicate_rmf
    dq k0_ippsHashDuplicate_rmf
    dq k1_ippsHashDuplicate_rmf

segment .text
global ippsHashDuplicate_rmf:function (ippsHashDuplicate_rmf.LEndippsHashDuplicate_rmf - ippsHashDuplicate_rmf)
.Lin_ippsHashDuplicate_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashDuplicate_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashDuplicate_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashDuplicate_rmf:
