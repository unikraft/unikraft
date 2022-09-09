%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpSetElement%+elf_symbol_type
extern n8_ippsGFpSetElement%+elf_symbol_type
extern y8_ippsGFpSetElement%+elf_symbol_type
extern e9_ippsGFpSetElement%+elf_symbol_type
extern l9_ippsGFpSetElement%+elf_symbol_type
extern n0_ippsGFpSetElement%+elf_symbol_type
extern k0_ippsGFpSetElement%+elf_symbol_type
extern k1_ippsGFpSetElement%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpSetElement
.Larraddr_ippsGFpSetElement:
    dq m7_ippsGFpSetElement
    dq n8_ippsGFpSetElement
    dq y8_ippsGFpSetElement
    dq e9_ippsGFpSetElement
    dq l9_ippsGFpSetElement
    dq n0_ippsGFpSetElement
    dq k0_ippsGFpSetElement
    dq k1_ippsGFpSetElement

segment .text
global ippsGFpSetElement:function (ippsGFpSetElement.LEndippsGFpSetElement - ippsGFpSetElement)
.Lin_ippsGFpSetElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpSetElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpSetElement]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpSetElement:
