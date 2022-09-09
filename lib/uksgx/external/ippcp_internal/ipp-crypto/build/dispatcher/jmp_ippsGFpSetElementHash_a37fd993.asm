%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpSetElementHash%+elf_symbol_type
extern n8_ippsGFpSetElementHash%+elf_symbol_type
extern y8_ippsGFpSetElementHash%+elf_symbol_type
extern e9_ippsGFpSetElementHash%+elf_symbol_type
extern l9_ippsGFpSetElementHash%+elf_symbol_type
extern n0_ippsGFpSetElementHash%+elf_symbol_type
extern k0_ippsGFpSetElementHash%+elf_symbol_type
extern k1_ippsGFpSetElementHash%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpSetElementHash
.Larraddr_ippsGFpSetElementHash:
    dq m7_ippsGFpSetElementHash
    dq n8_ippsGFpSetElementHash
    dq y8_ippsGFpSetElementHash
    dq e9_ippsGFpSetElementHash
    dq l9_ippsGFpSetElementHash
    dq n0_ippsGFpSetElementHash
    dq k0_ippsGFpSetElementHash
    dq k1_ippsGFpSetElementHash

segment .text
global ippsGFpSetElementHash:function (ippsGFpSetElementHash.LEndippsGFpSetElementHash - ippsGFpSetElementHash)
.Lin_ippsGFpSetElementHash:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpSetElementHash:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpSetElementHash]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpSetElementHash:
