%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECVerify%+elf_symbol_type
extern n8_ippsGFpECVerify%+elf_symbol_type
extern y8_ippsGFpECVerify%+elf_symbol_type
extern e9_ippsGFpECVerify%+elf_symbol_type
extern l9_ippsGFpECVerify%+elf_symbol_type
extern n0_ippsGFpECVerify%+elf_symbol_type
extern k0_ippsGFpECVerify%+elf_symbol_type
extern k1_ippsGFpECVerify%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECVerify
.Larraddr_ippsGFpECVerify:
    dq m7_ippsGFpECVerify
    dq n8_ippsGFpECVerify
    dq y8_ippsGFpECVerify
    dq e9_ippsGFpECVerify
    dq l9_ippsGFpECVerify
    dq n0_ippsGFpECVerify
    dq k0_ippsGFpECVerify
    dq k1_ippsGFpECVerify

segment .text
global ippsGFpECVerify:function (ippsGFpECVerify.LEndippsGFpECVerify - ippsGFpECVerify)
.Lin_ippsGFpECVerify:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECVerify:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECVerify]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECVerify:
