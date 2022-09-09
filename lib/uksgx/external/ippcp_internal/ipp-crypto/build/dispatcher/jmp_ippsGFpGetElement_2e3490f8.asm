%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpGetElement%+elf_symbol_type
extern n8_ippsGFpGetElement%+elf_symbol_type
extern y8_ippsGFpGetElement%+elf_symbol_type
extern e9_ippsGFpGetElement%+elf_symbol_type
extern l9_ippsGFpGetElement%+elf_symbol_type
extern n0_ippsGFpGetElement%+elf_symbol_type
extern k0_ippsGFpGetElement%+elf_symbol_type
extern k1_ippsGFpGetElement%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpGetElement
.Larraddr_ippsGFpGetElement:
    dq m7_ippsGFpGetElement
    dq n8_ippsGFpGetElement
    dq y8_ippsGFpGetElement
    dq e9_ippsGFpGetElement
    dq l9_ippsGFpGetElement
    dq n0_ippsGFpGetElement
    dq k0_ippsGFpGetElement
    dq k1_ippsGFpGetElement

segment .text
global ippsGFpGetElement:function (ippsGFpGetElement.LEndippsGFpGetElement - ippsGFpGetElement)
.Lin_ippsGFpGetElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpGetElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpGetElement]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpGetElement:
