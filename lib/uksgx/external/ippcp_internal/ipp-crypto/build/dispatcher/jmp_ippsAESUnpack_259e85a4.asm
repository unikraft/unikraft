%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESUnpack%+elf_symbol_type
extern n8_ippsAESUnpack%+elf_symbol_type
extern y8_ippsAESUnpack%+elf_symbol_type
extern e9_ippsAESUnpack%+elf_symbol_type
extern l9_ippsAESUnpack%+elf_symbol_type
extern n0_ippsAESUnpack%+elf_symbol_type
extern k0_ippsAESUnpack%+elf_symbol_type
extern k1_ippsAESUnpack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESUnpack
.Larraddr_ippsAESUnpack:
    dq m7_ippsAESUnpack
    dq n8_ippsAESUnpack
    dq y8_ippsAESUnpack
    dq e9_ippsAESUnpack
    dq l9_ippsAESUnpack
    dq n0_ippsAESUnpack
    dq k0_ippsAESUnpack
    dq k1_ippsAESUnpack

segment .text
global ippsAESUnpack:function (ippsAESUnpack.LEndippsAESUnpack - ippsAESUnpack)
.Lin_ippsAESUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESUnpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESUnpack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESUnpack:
