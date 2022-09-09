%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsARCFourGetSize%+elf_symbol_type
extern n8_ippsARCFourGetSize%+elf_symbol_type
extern y8_ippsARCFourGetSize%+elf_symbol_type
extern e9_ippsARCFourGetSize%+elf_symbol_type
extern l9_ippsARCFourGetSize%+elf_symbol_type
extern n0_ippsARCFourGetSize%+elf_symbol_type
extern k0_ippsARCFourGetSize%+elf_symbol_type
extern k1_ippsARCFourGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsARCFourGetSize
.Larraddr_ippsARCFourGetSize:
    dq m7_ippsARCFourGetSize
    dq n8_ippsARCFourGetSize
    dq y8_ippsARCFourGetSize
    dq e9_ippsARCFourGetSize
    dq l9_ippsARCFourGetSize
    dq n0_ippsARCFourGetSize
    dq k0_ippsARCFourGetSize
    dq k1_ippsARCFourGetSize

segment .text
global ippsARCFourGetSize:function (ippsARCFourGetSize.LEndippsARCFourGetSize - ippsARCFourGetSize)
.Lin_ippsARCFourGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsARCFourGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsARCFourGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsARCFourGetSize:
