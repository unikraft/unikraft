%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsARCFourCheckKey%+elf_symbol_type
extern n8_ippsARCFourCheckKey%+elf_symbol_type
extern y8_ippsARCFourCheckKey%+elf_symbol_type
extern e9_ippsARCFourCheckKey%+elf_symbol_type
extern l9_ippsARCFourCheckKey%+elf_symbol_type
extern n0_ippsARCFourCheckKey%+elf_symbol_type
extern k0_ippsARCFourCheckKey%+elf_symbol_type
extern k1_ippsARCFourCheckKey%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsARCFourCheckKey
.Larraddr_ippsARCFourCheckKey:
    dq m7_ippsARCFourCheckKey
    dq n8_ippsARCFourCheckKey
    dq y8_ippsARCFourCheckKey
    dq e9_ippsARCFourCheckKey
    dq l9_ippsARCFourCheckKey
    dq n0_ippsARCFourCheckKey
    dq k0_ippsARCFourCheckKey
    dq k1_ippsARCFourCheckKey

segment .text
global ippsARCFourCheckKey:function (ippsARCFourCheckKey.LEndippsARCFourCheckKey - ippsARCFourCheckKey)
.Lin_ippsARCFourCheckKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsARCFourCheckKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsARCFourCheckKey]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsARCFourCheckKey:
