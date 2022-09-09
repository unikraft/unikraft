%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetPointHash%+elf_symbol_type
extern n8_ippsGFpECSetPointHash%+elf_symbol_type
extern y8_ippsGFpECSetPointHash%+elf_symbol_type
extern e9_ippsGFpECSetPointHash%+elf_symbol_type
extern l9_ippsGFpECSetPointHash%+elf_symbol_type
extern n0_ippsGFpECSetPointHash%+elf_symbol_type
extern k0_ippsGFpECSetPointHash%+elf_symbol_type
extern k1_ippsGFpECSetPointHash%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetPointHash
.Larraddr_ippsGFpECSetPointHash:
    dq m7_ippsGFpECSetPointHash
    dq n8_ippsGFpECSetPointHash
    dq y8_ippsGFpECSetPointHash
    dq e9_ippsGFpECSetPointHash
    dq l9_ippsGFpECSetPointHash
    dq n0_ippsGFpECSetPointHash
    dq k0_ippsGFpECSetPointHash
    dq k1_ippsGFpECSetPointHash

segment .text
global ippsGFpECSetPointHash:function (ippsGFpECSetPointHash.LEndippsGFpECSetPointHash - ippsGFpECSetPointHash)
.Lin_ippsGFpECSetPointHash:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetPointHash:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetPointHash]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetPointHash:
