%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashFinal%+elf_symbol_type
extern n8_ippsHashFinal%+elf_symbol_type
extern y8_ippsHashFinal%+elf_symbol_type
extern e9_ippsHashFinal%+elf_symbol_type
extern l9_ippsHashFinal%+elf_symbol_type
extern n0_ippsHashFinal%+elf_symbol_type
extern k0_ippsHashFinal%+elf_symbol_type
extern k1_ippsHashFinal%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashFinal
.Larraddr_ippsHashFinal:
    dq m7_ippsHashFinal
    dq n8_ippsHashFinal
    dq y8_ippsHashFinal
    dq e9_ippsHashFinal
    dq l9_ippsHashFinal
    dq n0_ippsHashFinal
    dq k0_ippsHashFinal
    dq k1_ippsHashFinal

segment .text
global ippsHashFinal:function (ippsHashFinal.LEndippsHashFinal - ippsHashFinal)
.Lin_ippsHashFinal:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashFinal:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashFinal]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashFinal:
