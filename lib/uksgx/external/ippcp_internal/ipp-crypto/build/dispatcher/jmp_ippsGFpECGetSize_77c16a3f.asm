%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECGetSize%+elf_symbol_type
extern n8_ippsGFpECGetSize%+elf_symbol_type
extern y8_ippsGFpECGetSize%+elf_symbol_type
extern e9_ippsGFpECGetSize%+elf_symbol_type
extern l9_ippsGFpECGetSize%+elf_symbol_type
extern n0_ippsGFpECGetSize%+elf_symbol_type
extern k0_ippsGFpECGetSize%+elf_symbol_type
extern k1_ippsGFpECGetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECGetSize
.Larraddr_ippsGFpECGetSize:
    dq m7_ippsGFpECGetSize
    dq n8_ippsGFpECGetSize
    dq y8_ippsGFpECGetSize
    dq e9_ippsGFpECGetSize
    dq l9_ippsGFpECGetSize
    dq n0_ippsGFpECGetSize
    dq k0_ippsGFpECGetSize
    dq k1_ippsGFpECGetSize

segment .text
global ippsGFpECGetSize:function (ippsGFpECGetSize.LEndippsGFpECGetSize - ippsGFpECGetSize)
.Lin_ippsGFpECGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECGetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECGetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECGetSize:
