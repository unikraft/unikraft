%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern n8_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern y8_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern e9_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern l9_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern n0_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern k0_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern k1_ippsGFpECBindGxyTblStd256r1%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECBindGxyTblStd256r1
.Larraddr_ippsGFpECBindGxyTblStd256r1:
    dq m7_ippsGFpECBindGxyTblStd256r1
    dq n8_ippsGFpECBindGxyTblStd256r1
    dq y8_ippsGFpECBindGxyTblStd256r1
    dq e9_ippsGFpECBindGxyTblStd256r1
    dq l9_ippsGFpECBindGxyTblStd256r1
    dq n0_ippsGFpECBindGxyTblStd256r1
    dq k0_ippsGFpECBindGxyTblStd256r1
    dq k1_ippsGFpECBindGxyTblStd256r1

segment .text
global ippsGFpECBindGxyTblStd256r1:function (ippsGFpECBindGxyTblStd256r1.LEndippsGFpECBindGxyTblStd256r1 - ippsGFpECBindGxyTblStd256r1)
.Lin_ippsGFpECBindGxyTblStd256r1:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECBindGxyTblStd256r1:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECBindGxyTblStd256r1]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECBindGxyTblStd256r1:
