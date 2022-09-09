%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECSetPointRegular%+elf_symbol_type
extern n8_ippsGFpECSetPointRegular%+elf_symbol_type
extern y8_ippsGFpECSetPointRegular%+elf_symbol_type
extern e9_ippsGFpECSetPointRegular%+elf_symbol_type
extern l9_ippsGFpECSetPointRegular%+elf_symbol_type
extern n0_ippsGFpECSetPointRegular%+elf_symbol_type
extern k0_ippsGFpECSetPointRegular%+elf_symbol_type
extern k1_ippsGFpECSetPointRegular%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECSetPointRegular
.Larraddr_ippsGFpECSetPointRegular:
    dq m7_ippsGFpECSetPointRegular
    dq n8_ippsGFpECSetPointRegular
    dq y8_ippsGFpECSetPointRegular
    dq e9_ippsGFpECSetPointRegular
    dq l9_ippsGFpECSetPointRegular
    dq n0_ippsGFpECSetPointRegular
    dq k0_ippsGFpECSetPointRegular
    dq k1_ippsGFpECSetPointRegular

segment .text
global ippsGFpECSetPointRegular:function (ippsGFpECSetPointRegular.LEndippsGFpECSetPointRegular - ippsGFpECSetPointRegular)
.Lin_ippsGFpECSetPointRegular:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECSetPointRegular:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECSetPointRegular]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECSetPointRegular:
