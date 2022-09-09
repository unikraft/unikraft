%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpxInitBinomial%+elf_symbol_type
extern n8_ippsGFpxInitBinomial%+elf_symbol_type
extern y8_ippsGFpxInitBinomial%+elf_symbol_type
extern e9_ippsGFpxInitBinomial%+elf_symbol_type
extern l9_ippsGFpxInitBinomial%+elf_symbol_type
extern n0_ippsGFpxInitBinomial%+elf_symbol_type
extern k0_ippsGFpxInitBinomial%+elf_symbol_type
extern k1_ippsGFpxInitBinomial%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpxInitBinomial
.Larraddr_ippsGFpxInitBinomial:
    dq m7_ippsGFpxInitBinomial
    dq n8_ippsGFpxInitBinomial
    dq y8_ippsGFpxInitBinomial
    dq e9_ippsGFpxInitBinomial
    dq l9_ippsGFpxInitBinomial
    dq n0_ippsGFpxInitBinomial
    dq k0_ippsGFpxInitBinomial
    dq k1_ippsGFpxInitBinomial

segment .text
global ippsGFpxInitBinomial:function (ippsGFpxInitBinomial.LEndippsGFpxInitBinomial - ippsGFpxInitBinomial)
.Lin_ippsGFpxInitBinomial:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpxInitBinomial:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpxInitBinomial]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpxInitBinomial:
