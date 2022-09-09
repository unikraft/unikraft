%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECTstKeyPair%+elf_symbol_type
extern n8_ippsGFpECTstKeyPair%+elf_symbol_type
extern y8_ippsGFpECTstKeyPair%+elf_symbol_type
extern e9_ippsGFpECTstKeyPair%+elf_symbol_type
extern l9_ippsGFpECTstKeyPair%+elf_symbol_type
extern n0_ippsGFpECTstKeyPair%+elf_symbol_type
extern k0_ippsGFpECTstKeyPair%+elf_symbol_type
extern k1_ippsGFpECTstKeyPair%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECTstKeyPair
.Larraddr_ippsGFpECTstKeyPair:
    dq m7_ippsGFpECTstKeyPair
    dq n8_ippsGFpECTstKeyPair
    dq y8_ippsGFpECTstKeyPair
    dq e9_ippsGFpECTstKeyPair
    dq l9_ippsGFpECTstKeyPair
    dq n0_ippsGFpECTstKeyPair
    dq k0_ippsGFpECTstKeyPair
    dq k1_ippsGFpECTstKeyPair

segment .text
global ippsGFpECTstKeyPair:function (ippsGFpECTstKeyPair.LEndippsGFpECTstKeyPair - ippsGFpECTstKeyPair)
.Lin_ippsGFpECTstKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECTstKeyPair:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECTstKeyPair]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECTstKeyPair:
