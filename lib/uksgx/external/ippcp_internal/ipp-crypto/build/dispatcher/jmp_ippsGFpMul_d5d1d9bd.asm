%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpMul%+elf_symbol_type
extern n8_ippsGFpMul%+elf_symbol_type
extern y8_ippsGFpMul%+elf_symbol_type
extern e9_ippsGFpMul%+elf_symbol_type
extern l9_ippsGFpMul%+elf_symbol_type
extern n0_ippsGFpMul%+elf_symbol_type
extern k0_ippsGFpMul%+elf_symbol_type
extern k1_ippsGFpMul%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpMul
.Larraddr_ippsGFpMul:
    dq m7_ippsGFpMul
    dq n8_ippsGFpMul
    dq y8_ippsGFpMul
    dq e9_ippsGFpMul
    dq l9_ippsGFpMul
    dq n0_ippsGFpMul
    dq k0_ippsGFpMul
    dq k1_ippsGFpMul

segment .text
global ippsGFpMul:function (ippsGFpMul.LEndippsGFpMul - ippsGFpMul)
.Lin_ippsGFpMul:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpMul:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpMul]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpMul:
