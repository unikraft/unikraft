%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpSetElementRegular%+elf_symbol_type
extern n8_ippsGFpSetElementRegular%+elf_symbol_type
extern y8_ippsGFpSetElementRegular%+elf_symbol_type
extern e9_ippsGFpSetElementRegular%+elf_symbol_type
extern l9_ippsGFpSetElementRegular%+elf_symbol_type
extern n0_ippsGFpSetElementRegular%+elf_symbol_type
extern k0_ippsGFpSetElementRegular%+elf_symbol_type
extern k1_ippsGFpSetElementRegular%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpSetElementRegular
.Larraddr_ippsGFpSetElementRegular:
    dq m7_ippsGFpSetElementRegular
    dq n8_ippsGFpSetElementRegular
    dq y8_ippsGFpSetElementRegular
    dq e9_ippsGFpSetElementRegular
    dq l9_ippsGFpSetElementRegular
    dq n0_ippsGFpSetElementRegular
    dq k0_ippsGFpSetElementRegular
    dq k1_ippsGFpSetElementRegular

segment .text
global ippsGFpSetElementRegular:function (ippsGFpSetElementRegular.LEndippsGFpSetElementRegular - ippsGFpSetElementRegular)
.Lin_ippsGFpSetElementRegular:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpSetElementRegular:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpSetElementRegular]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpSetElementRegular:
