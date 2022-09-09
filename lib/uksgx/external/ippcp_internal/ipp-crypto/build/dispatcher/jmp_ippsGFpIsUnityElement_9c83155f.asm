%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpIsUnityElement%+elf_symbol_type
extern n8_ippsGFpIsUnityElement%+elf_symbol_type
extern y8_ippsGFpIsUnityElement%+elf_symbol_type
extern e9_ippsGFpIsUnityElement%+elf_symbol_type
extern l9_ippsGFpIsUnityElement%+elf_symbol_type
extern n0_ippsGFpIsUnityElement%+elf_symbol_type
extern k0_ippsGFpIsUnityElement%+elf_symbol_type
extern k1_ippsGFpIsUnityElement%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpIsUnityElement
.Larraddr_ippsGFpIsUnityElement:
    dq m7_ippsGFpIsUnityElement
    dq n8_ippsGFpIsUnityElement
    dq y8_ippsGFpIsUnityElement
    dq e9_ippsGFpIsUnityElement
    dq l9_ippsGFpIsUnityElement
    dq n0_ippsGFpIsUnityElement
    dq k0_ippsGFpIsUnityElement
    dq k1_ippsGFpIsUnityElement

segment .text
global ippsGFpIsUnityElement:function (ippsGFpIsUnityElement.LEndippsGFpIsUnityElement - ippsGFpIsUnityElement)
.Lin_ippsGFpIsUnityElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpIsUnityElement:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpIsUnityElement]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpIsUnityElement:
