%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpCpyElement%+elf_symbol_type
extern n8_ippsGFpCpyElement%+elf_symbol_type
extern y8_ippsGFpCpyElement%+elf_symbol_type
extern e9_ippsGFpCpyElement%+elf_symbol_type
extern l9_ippsGFpCpyElement%+elf_symbol_type
extern n0_ippsGFpCpyElement%+elf_symbol_type
extern k0_ippsGFpCpyElement%+elf_symbol_type
extern k1_ippsGFpCpyElement%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpCpyElement
.Larraddr_ippsGFpCpyElement:
    dq m7_ippsGFpCpyElement
    dq n8_ippsGFpCpyElement
    dq y8_ippsGFpCpyElement
    dq e9_ippsGFpCpyElement
    dq l9_ippsGFpCpyElement
    dq n0_ippsGFpCpyElement
    dq k0_ippsGFpCpyElement
    dq k1_ippsGFpCpyElement

segment .text
global ippsGFpCpyElement:function (ippsGFpCpyElement.LEndippsGFpCpyElement - ippsGFpCpyElement)
.Lin_ippsGFpCpyElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpCpyElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpCpyElement]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpCpyElement:
