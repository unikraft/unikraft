%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpConj%+elf_symbol_type
extern n8_ippsGFpConj%+elf_symbol_type
extern y8_ippsGFpConj%+elf_symbol_type
extern e9_ippsGFpConj%+elf_symbol_type
extern l9_ippsGFpConj%+elf_symbol_type
extern n0_ippsGFpConj%+elf_symbol_type
extern k0_ippsGFpConj%+elf_symbol_type
extern k1_ippsGFpConj%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpConj
.Larraddr_ippsGFpConj:
    dq m7_ippsGFpConj
    dq n8_ippsGFpConj
    dq y8_ippsGFpConj
    dq e9_ippsGFpConj
    dq l9_ippsGFpConj
    dq n0_ippsGFpConj
    dq k0_ippsGFpConj
    dq k1_ippsGFpConj

segment .text
global ippsGFpConj:function (ippsGFpConj.LEndippsGFpConj - ippsGFpConj)
.Lin_ippsGFpConj:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpConj:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpConj]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpConj:
