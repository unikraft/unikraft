%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECPublicKey%+elf_symbol_type
extern n8_ippsGFpECPublicKey%+elf_symbol_type
extern y8_ippsGFpECPublicKey%+elf_symbol_type
extern e9_ippsGFpECPublicKey%+elf_symbol_type
extern l9_ippsGFpECPublicKey%+elf_symbol_type
extern n0_ippsGFpECPublicKey%+elf_symbol_type
extern k0_ippsGFpECPublicKey%+elf_symbol_type
extern k1_ippsGFpECPublicKey%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECPublicKey
.Larraddr_ippsGFpECPublicKey:
    dq m7_ippsGFpECPublicKey
    dq n8_ippsGFpECPublicKey
    dq y8_ippsGFpECPublicKey
    dq e9_ippsGFpECPublicKey
    dq l9_ippsGFpECPublicKey
    dq n0_ippsGFpECPublicKey
    dq k0_ippsGFpECPublicKey
    dq k1_ippsGFpECPublicKey

segment .text
global ippsGFpECPublicKey:function (ippsGFpECPublicKey.LEndippsGFpECPublicKey - ippsGFpECPublicKey)
.Lin_ippsGFpECPublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECPublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECPublicKey]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECPublicKey:
