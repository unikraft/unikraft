%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsARCFourPack%+elf_symbol_type
extern n8_ippsARCFourPack%+elf_symbol_type
extern y8_ippsARCFourPack%+elf_symbol_type
extern e9_ippsARCFourPack%+elf_symbol_type
extern l9_ippsARCFourPack%+elf_symbol_type
extern n0_ippsARCFourPack%+elf_symbol_type
extern k0_ippsARCFourPack%+elf_symbol_type
extern k1_ippsARCFourPack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsARCFourPack
.Larraddr_ippsARCFourPack:
    dq m7_ippsARCFourPack
    dq n8_ippsARCFourPack
    dq y8_ippsARCFourPack
    dq e9_ippsARCFourPack
    dq l9_ippsARCFourPack
    dq n0_ippsARCFourPack
    dq k0_ippsARCFourPack
    dq k1_ippsARCFourPack

segment .text
global ippsARCFourPack:function (ippsARCFourPack.LEndippsARCFourPack - ippsARCFourPack)
.Lin_ippsARCFourPack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsARCFourPack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsARCFourPack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsARCFourPack:
