%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpECGetPointRegular%+elf_symbol_type
extern n8_ippsGFpECGetPointRegular%+elf_symbol_type
extern y8_ippsGFpECGetPointRegular%+elf_symbol_type
extern e9_ippsGFpECGetPointRegular%+elf_symbol_type
extern l9_ippsGFpECGetPointRegular%+elf_symbol_type
extern n0_ippsGFpECGetPointRegular%+elf_symbol_type
extern k0_ippsGFpECGetPointRegular%+elf_symbol_type
extern k1_ippsGFpECGetPointRegular%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpECGetPointRegular
.Larraddr_ippsGFpECGetPointRegular:
    dq m7_ippsGFpECGetPointRegular
    dq n8_ippsGFpECGetPointRegular
    dq y8_ippsGFpECGetPointRegular
    dq e9_ippsGFpECGetPointRegular
    dq l9_ippsGFpECGetPointRegular
    dq n0_ippsGFpECGetPointRegular
    dq k0_ippsGFpECGetPointRegular
    dq k1_ippsGFpECGetPointRegular

segment .text
global ippsGFpECGetPointRegular:function (ippsGFpECGetPointRegular.LEndippsGFpECGetPointRegular - ippsGFpECGetPointRegular)
.Lin_ippsGFpECGetPointRegular:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpECGetPointRegular:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpECGetPointRegular]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpECGetPointRegular:
