%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashMethod_SHA1%+elf_symbol_type
extern n8_ippsHashMethod_SHA1%+elf_symbol_type
extern y8_ippsHashMethod_SHA1%+elf_symbol_type
extern e9_ippsHashMethod_SHA1%+elf_symbol_type
extern l9_ippsHashMethod_SHA1%+elf_symbol_type
extern n0_ippsHashMethod_SHA1%+elf_symbol_type
extern k0_ippsHashMethod_SHA1%+elf_symbol_type
extern k1_ippsHashMethod_SHA1%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashMethod_SHA1
.Larraddr_ippsHashMethod_SHA1:
    dq m7_ippsHashMethod_SHA1
    dq n8_ippsHashMethod_SHA1
    dq y8_ippsHashMethod_SHA1
    dq e9_ippsHashMethod_SHA1
    dq l9_ippsHashMethod_SHA1
    dq n0_ippsHashMethod_SHA1
    dq k0_ippsHashMethod_SHA1
    dq k1_ippsHashMethod_SHA1

segment .text
global ippsHashMethod_SHA1:function (ippsHashMethod_SHA1.LEndippsHashMethod_SHA1 - ippsHashMethod_SHA1)
.Lin_ippsHashMethod_SHA1:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashMethod_SHA1:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashMethod_SHA1]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashMethod_SHA1:
