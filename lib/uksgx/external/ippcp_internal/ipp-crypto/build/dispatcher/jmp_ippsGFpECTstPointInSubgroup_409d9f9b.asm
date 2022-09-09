%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern n8_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern y8_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern e9_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern l9_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern n0_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern k0_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern k1_ippsGFpECTstPointInSubgroup%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECTstPointInSubgroup
.Larraddr_ippsGFpECTstPointInSubgroup:
    dq m7_ippsGFpECTstPointInSubgroup
    dq n8_ippsGFpECTstPointInSubgroup
    dq y8_ippsGFpECTstPointInSubgroup
    dq e9_ippsGFpECTstPointInSubgroup
    dq l9_ippsGFpECTstPointInSubgroup
    dq n0_ippsGFpECTstPointInSubgroup
    dq k0_ippsGFpECTstPointInSubgroup
    dq k1_ippsGFpECTstPointInSubgroup

segment .text
global ippsGFpECTstPointInSubgroup:function (ippsGFpECTstPointInSubgroup.LEndippsGFpECTstPointInSubgroup - ippsGFpECTstPointInSubgroup)
.Lin_ippsGFpECTstPointInSubgroup:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECTstPointInSubgroup:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECTstPointInSubgroup]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECTstPointInSubgroup:
