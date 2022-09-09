%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashInit_rmf%+elf_symbol_type
extern n8_ippsHashInit_rmf%+elf_symbol_type
extern y8_ippsHashInit_rmf%+elf_symbol_type
extern e9_ippsHashInit_rmf%+elf_symbol_type
extern l9_ippsHashInit_rmf%+elf_symbol_type
extern n0_ippsHashInit_rmf%+elf_symbol_type
extern k0_ippsHashInit_rmf%+elf_symbol_type
extern k1_ippsHashInit_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashInit_rmf
.Larraddr_ippsHashInit_rmf:
    dq m7_ippsHashInit_rmf
    dq n8_ippsHashInit_rmf
    dq y8_ippsHashInit_rmf
    dq e9_ippsHashInit_rmf
    dq l9_ippsHashInit_rmf
    dq n0_ippsHashInit_rmf
    dq k0_ippsHashInit_rmf
    dq k1_ippsHashInit_rmf

segment .text
global ippsHashInit_rmf:function (ippsHashInit_rmf.LEndippsHashInit_rmf - ippsHashInit_rmf)
.Lin_ippsHashInit_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashInit_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashInit_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashInit_rmf:
