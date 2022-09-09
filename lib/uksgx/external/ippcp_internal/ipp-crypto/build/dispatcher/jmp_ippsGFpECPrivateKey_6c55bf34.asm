%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECPrivateKey%+elf_symbol_type
extern n8_ippsGFpECPrivateKey%+elf_symbol_type
extern y8_ippsGFpECPrivateKey%+elf_symbol_type
extern e9_ippsGFpECPrivateKey%+elf_symbol_type
extern l9_ippsGFpECPrivateKey%+elf_symbol_type
extern n0_ippsGFpECPrivateKey%+elf_symbol_type
extern k0_ippsGFpECPrivateKey%+elf_symbol_type
extern k1_ippsGFpECPrivateKey%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECPrivateKey
.Larraddr_ippsGFpECPrivateKey:
    dq m7_ippsGFpECPrivateKey
    dq n8_ippsGFpECPrivateKey
    dq y8_ippsGFpECPrivateKey
    dq e9_ippsGFpECPrivateKey
    dq l9_ippsGFpECPrivateKey
    dq n0_ippsGFpECPrivateKey
    dq k0_ippsGFpECPrivateKey
    dq k1_ippsGFpECPrivateKey

segment .text
global ippsGFpECPrivateKey:function (ippsGFpECPrivateKey.LEndippsGFpECPrivateKey - ippsGFpECPrivateKey)
.Lin_ippsGFpECPrivateKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECPrivateKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECPrivateKey]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECPrivateKey:
