%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern n8_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern y8_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern e9_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern l9_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern n0_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern k0_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern k1_ippsHashMethod_SHA256_TT%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashMethod_SHA256_TT
.Larraddr_ippsHashMethod_SHA256_TT:
    dq m7_ippsHashMethod_SHA256_TT
    dq n8_ippsHashMethod_SHA256_TT
    dq y8_ippsHashMethod_SHA256_TT
    dq e9_ippsHashMethod_SHA256_TT
    dq l9_ippsHashMethod_SHA256_TT
    dq n0_ippsHashMethod_SHA256_TT
    dq k0_ippsHashMethod_SHA256_TT
    dq k1_ippsHashMethod_SHA256_TT

segment .text
global ippsHashMethod_SHA256_TT:function (ippsHashMethod_SHA256_TT.LEndippsHashMethod_SHA256_TT - ippsHashMethod_SHA256_TT)
.Lin_ippsHashMethod_SHA256_TT:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashMethod_SHA256_TT:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashMethod_SHA256_TT]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashMethod_SHA256_TT:
