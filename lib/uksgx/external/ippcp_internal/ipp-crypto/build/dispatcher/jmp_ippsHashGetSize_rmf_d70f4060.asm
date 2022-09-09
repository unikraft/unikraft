%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashGetSize_rmf%+elf_symbol_type
extern n8_ippsHashGetSize_rmf%+elf_symbol_type
extern y8_ippsHashGetSize_rmf%+elf_symbol_type
extern e9_ippsHashGetSize_rmf%+elf_symbol_type
extern l9_ippsHashGetSize_rmf%+elf_symbol_type
extern n0_ippsHashGetSize_rmf%+elf_symbol_type
extern k0_ippsHashGetSize_rmf%+elf_symbol_type
extern k1_ippsHashGetSize_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashGetSize_rmf
.Larraddr_ippsHashGetSize_rmf:
    dq m7_ippsHashGetSize_rmf
    dq n8_ippsHashGetSize_rmf
    dq y8_ippsHashGetSize_rmf
    dq e9_ippsHashGetSize_rmf
    dq l9_ippsHashGetSize_rmf
    dq n0_ippsHashGetSize_rmf
    dq k0_ippsHashGetSize_rmf
    dq k1_ippsHashGetSize_rmf

segment .text
global ippsHashGetSize_rmf:function (ippsHashGetSize_rmf.LEndippsHashGetSize_rmf - ippsHashGetSize_rmf)
.Lin_ippsHashGetSize_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashGetSize_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashGetSize_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashGetSize_rmf:
