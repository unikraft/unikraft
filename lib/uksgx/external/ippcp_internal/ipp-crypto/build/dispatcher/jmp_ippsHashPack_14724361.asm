%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHashPack%+elf_symbol_type
extern n8_ippsHashPack%+elf_symbol_type
extern y8_ippsHashPack%+elf_symbol_type
extern e9_ippsHashPack%+elf_symbol_type
extern l9_ippsHashPack%+elf_symbol_type
extern n0_ippsHashPack%+elf_symbol_type
extern k0_ippsHashPack%+elf_symbol_type
extern k1_ippsHashPack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHashPack
.Larraddr_ippsHashPack:
    dq m7_ippsHashPack
    dq n8_ippsHashPack
    dq y8_ippsHashPack
    dq e9_ippsHashPack
    dq l9_ippsHashPack
    dq n0_ippsHashPack
    dq k0_ippsHashPack
    dq k1_ippsHashPack

segment .text
global ippsHashPack:function (ippsHashPack.LEndippsHashPack - ippsHashPack)
.Lin_ippsHashPack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHashPack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHashPack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHashPack:
