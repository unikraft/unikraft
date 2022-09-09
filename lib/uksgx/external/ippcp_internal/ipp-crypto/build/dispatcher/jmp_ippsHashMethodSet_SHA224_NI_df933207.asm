%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern n8_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern y8_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern e9_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern l9_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern n0_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern k0_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern k1_ippsHashMethodSet_SHA224_NI%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashMethodSet_SHA224_NI
.Larraddr_ippsHashMethodSet_SHA224_NI:
    dq m7_ippsHashMethodSet_SHA224_NI
    dq n8_ippsHashMethodSet_SHA224_NI
    dq y8_ippsHashMethodSet_SHA224_NI
    dq e9_ippsHashMethodSet_SHA224_NI
    dq l9_ippsHashMethodSet_SHA224_NI
    dq n0_ippsHashMethodSet_SHA224_NI
    dq k0_ippsHashMethodSet_SHA224_NI
    dq k1_ippsHashMethodSet_SHA224_NI

segment .text
global ippsHashMethodSet_SHA224_NI:function (ippsHashMethodSet_SHA224_NI.LEndippsHashMethodSet_SHA224_NI - ippsHashMethodSet_SHA224_NI)
.Lin_ippsHashMethodSet_SHA224_NI:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashMethodSet_SHA224_NI:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashMethodSet_SHA224_NI]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashMethodSet_SHA224_NI:
