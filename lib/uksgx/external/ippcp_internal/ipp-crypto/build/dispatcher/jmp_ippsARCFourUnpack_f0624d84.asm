%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsARCFourUnpack%+elf_symbol_type
extern n8_ippsARCFourUnpack%+elf_symbol_type
extern y8_ippsARCFourUnpack%+elf_symbol_type
extern e9_ippsARCFourUnpack%+elf_symbol_type
extern l9_ippsARCFourUnpack%+elf_symbol_type
extern n0_ippsARCFourUnpack%+elf_symbol_type
extern k0_ippsARCFourUnpack%+elf_symbol_type
extern k1_ippsARCFourUnpack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsARCFourUnpack
.Larraddr_ippsARCFourUnpack:
    dq m7_ippsARCFourUnpack
    dq n8_ippsARCFourUnpack
    dq y8_ippsARCFourUnpack
    dq e9_ippsARCFourUnpack
    dq l9_ippsARCFourUnpack
    dq n0_ippsARCFourUnpack
    dq k0_ippsARCFourUnpack
    dq k1_ippsARCFourUnpack

segment .text
global ippsARCFourUnpack:function (ippsARCFourUnpack.LEndippsARCFourUnpack - ippsARCFourUnpack)
.Lin_ippsARCFourUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsARCFourUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsARCFourUnpack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsARCFourUnpack:
