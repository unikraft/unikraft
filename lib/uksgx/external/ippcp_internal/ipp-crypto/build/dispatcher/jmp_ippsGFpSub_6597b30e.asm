%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpSub%+elf_symbol_type
extern n8_ippsGFpSub%+elf_symbol_type
extern y8_ippsGFpSub%+elf_symbol_type
extern e9_ippsGFpSub%+elf_symbol_type
extern l9_ippsGFpSub%+elf_symbol_type
extern n0_ippsGFpSub%+elf_symbol_type
extern k0_ippsGFpSub%+elf_symbol_type
extern k1_ippsGFpSub%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpSub
.Larraddr_ippsGFpSub:
    dq m7_ippsGFpSub
    dq n8_ippsGFpSub
    dq y8_ippsGFpSub
    dq e9_ippsGFpSub
    dq l9_ippsGFpSub
    dq n0_ippsGFpSub
    dq k0_ippsGFpSub
    dq k1_ippsGFpSub

segment .text
global ippsGFpSub:function (ippsGFpSub.LEndippsGFpSub - ippsGFpSub)
.Lin_ippsGFpSub:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpSub:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpSub]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpSub:
