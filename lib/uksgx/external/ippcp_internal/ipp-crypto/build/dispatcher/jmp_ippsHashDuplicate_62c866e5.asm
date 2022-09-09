%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashDuplicate%+elf_symbol_type
extern n8_ippsHashDuplicate%+elf_symbol_type
extern y8_ippsHashDuplicate%+elf_symbol_type
extern e9_ippsHashDuplicate%+elf_symbol_type
extern l9_ippsHashDuplicate%+elf_symbol_type
extern n0_ippsHashDuplicate%+elf_symbol_type
extern k0_ippsHashDuplicate%+elf_symbol_type
extern k1_ippsHashDuplicate%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashDuplicate
.Larraddr_ippsHashDuplicate:
    dq m7_ippsHashDuplicate
    dq n8_ippsHashDuplicate
    dq y8_ippsHashDuplicate
    dq e9_ippsHashDuplicate
    dq l9_ippsHashDuplicate
    dq n0_ippsHashDuplicate
    dq k0_ippsHashDuplicate
    dq k1_ippsHashDuplicate

segment .text
global ippsHashDuplicate:function (ippsHashDuplicate.LEndippsHashDuplicate - ippsHashDuplicate)
.Lin_ippsHashDuplicate:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashDuplicate:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashDuplicate]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashDuplicate:
