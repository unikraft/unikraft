%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSignNR%+elf_symbol_type
extern n8_ippsGFpECSignNR%+elf_symbol_type
extern y8_ippsGFpECSignNR%+elf_symbol_type
extern e9_ippsGFpECSignNR%+elf_symbol_type
extern l9_ippsGFpECSignNR%+elf_symbol_type
extern n0_ippsGFpECSignNR%+elf_symbol_type
extern k0_ippsGFpECSignNR%+elf_symbol_type
extern k1_ippsGFpECSignNR%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSignNR
.Larraddr_ippsGFpECSignNR:
    dq m7_ippsGFpECSignNR
    dq n8_ippsGFpECSignNR
    dq y8_ippsGFpECSignNR
    dq e9_ippsGFpECSignNR
    dq l9_ippsGFpECSignNR
    dq n0_ippsGFpECSignNR
    dq k0_ippsGFpECSignNR
    dq k1_ippsGFpECSignNR

segment .text
global ippsGFpECSignNR:function (ippsGFpECSignNR.LEndippsGFpECSignNR - ippsGFpECSignNR)
.Lin_ippsGFpECSignNR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSignNR:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSignNR]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSignNR:
