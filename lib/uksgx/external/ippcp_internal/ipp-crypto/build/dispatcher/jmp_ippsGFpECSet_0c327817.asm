%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSet%+elf_symbol_type
extern n8_ippsGFpECSet%+elf_symbol_type
extern y8_ippsGFpECSet%+elf_symbol_type
extern e9_ippsGFpECSet%+elf_symbol_type
extern l9_ippsGFpECSet%+elf_symbol_type
extern n0_ippsGFpECSet%+elf_symbol_type
extern k0_ippsGFpECSet%+elf_symbol_type
extern k1_ippsGFpECSet%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSet
.Larraddr_ippsGFpECSet:
    dq m7_ippsGFpECSet
    dq n8_ippsGFpECSet
    dq y8_ippsGFpECSet
    dq e9_ippsGFpECSet
    dq l9_ippsGFpECSet
    dq n0_ippsGFpECSet
    dq k0_ippsGFpECSet
    dq k1_ippsGFpECSet

segment .text
global ippsGFpECSet:function (ippsGFpECSet.LEndippsGFpECSet - ippsGFpECSet)
.Lin_ippsGFpECSet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSet]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSet:
