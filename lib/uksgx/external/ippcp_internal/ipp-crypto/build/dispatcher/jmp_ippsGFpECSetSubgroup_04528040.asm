%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetSubgroup%+elf_symbol_type
extern n8_ippsGFpECSetSubgroup%+elf_symbol_type
extern y8_ippsGFpECSetSubgroup%+elf_symbol_type
extern e9_ippsGFpECSetSubgroup%+elf_symbol_type
extern l9_ippsGFpECSetSubgroup%+elf_symbol_type
extern n0_ippsGFpECSetSubgroup%+elf_symbol_type
extern k0_ippsGFpECSetSubgroup%+elf_symbol_type
extern k1_ippsGFpECSetSubgroup%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetSubgroup
.Larraddr_ippsGFpECSetSubgroup:
    dq m7_ippsGFpECSetSubgroup
    dq n8_ippsGFpECSetSubgroup
    dq y8_ippsGFpECSetSubgroup
    dq e9_ippsGFpECSetSubgroup
    dq l9_ippsGFpECSetSubgroup
    dq n0_ippsGFpECSetSubgroup
    dq k0_ippsGFpECSetSubgroup
    dq k1_ippsGFpECSetSubgroup

segment .text
global ippsGFpECSetSubgroup:function (ippsGFpECSetSubgroup.LEndippsGFpECSetSubgroup - ippsGFpECSetSubgroup)
.Lin_ippsGFpECSetSubgroup:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetSubgroup:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetSubgroup]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetSubgroup:
