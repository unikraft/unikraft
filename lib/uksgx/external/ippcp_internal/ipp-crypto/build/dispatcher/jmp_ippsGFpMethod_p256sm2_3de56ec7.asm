%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpMethod_p256sm2%+elf_symbol_type
extern n8_ippsGFpMethod_p256sm2%+elf_symbol_type
extern y8_ippsGFpMethod_p256sm2%+elf_symbol_type
extern e9_ippsGFpMethod_p256sm2%+elf_symbol_type
extern l9_ippsGFpMethod_p256sm2%+elf_symbol_type
extern n0_ippsGFpMethod_p256sm2%+elf_symbol_type
extern k0_ippsGFpMethod_p256sm2%+elf_symbol_type
extern k1_ippsGFpMethod_p256sm2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpMethod_p256sm2
.Larraddr_ippsGFpMethod_p256sm2:
    dq m7_ippsGFpMethod_p256sm2
    dq n8_ippsGFpMethod_p256sm2
    dq y8_ippsGFpMethod_p256sm2
    dq e9_ippsGFpMethod_p256sm2
    dq l9_ippsGFpMethod_p256sm2
    dq n0_ippsGFpMethod_p256sm2
    dq k0_ippsGFpMethod_p256sm2
    dq k1_ippsGFpMethod_p256sm2

segment .text
global ippsGFpMethod_p256sm2:function (ippsGFpMethod_p256sm2.LEndippsGFpMethod_p256sm2 - ippsGFpMethod_p256sm2)
.Lin_ippsGFpMethod_p256sm2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpMethod_p256sm2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpMethod_p256sm2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpMethod_p256sm2:
