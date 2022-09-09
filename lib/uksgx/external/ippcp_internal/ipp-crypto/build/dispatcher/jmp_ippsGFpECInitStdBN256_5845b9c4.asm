%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECInitStdBN256%+elf_symbol_type
extern n8_ippsGFpECInitStdBN256%+elf_symbol_type
extern y8_ippsGFpECInitStdBN256%+elf_symbol_type
extern e9_ippsGFpECInitStdBN256%+elf_symbol_type
extern l9_ippsGFpECInitStdBN256%+elf_symbol_type
extern n0_ippsGFpECInitStdBN256%+elf_symbol_type
extern k0_ippsGFpECInitStdBN256%+elf_symbol_type
extern k1_ippsGFpECInitStdBN256%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECInitStdBN256
.Larraddr_ippsGFpECInitStdBN256:
    dq m7_ippsGFpECInitStdBN256
    dq n8_ippsGFpECInitStdBN256
    dq y8_ippsGFpECInitStdBN256
    dq e9_ippsGFpECInitStdBN256
    dq l9_ippsGFpECInitStdBN256
    dq n0_ippsGFpECInitStdBN256
    dq k0_ippsGFpECInitStdBN256
    dq k1_ippsGFpECInitStdBN256

segment .text
global ippsGFpECInitStdBN256:function (ippsGFpECInitStdBN256.LEndippsGFpECInitStdBN256 - ippsGFpECInitStdBN256)
.Lin_ippsGFpECInitStdBN256:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECInitStdBN256:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECInitStdBN256]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECInitStdBN256:
