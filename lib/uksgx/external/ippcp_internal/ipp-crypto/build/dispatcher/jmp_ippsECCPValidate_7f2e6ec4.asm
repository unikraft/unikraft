%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPValidate%+elf_symbol_type
extern n8_ippsECCPValidate%+elf_symbol_type
extern y8_ippsECCPValidate%+elf_symbol_type
extern e9_ippsECCPValidate%+elf_symbol_type
extern l9_ippsECCPValidate%+elf_symbol_type
extern n0_ippsECCPValidate%+elf_symbol_type
extern k0_ippsECCPValidate%+elf_symbol_type
extern k1_ippsECCPValidate%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPValidate
.Larraddr_ippsECCPValidate:
    dq m7_ippsECCPValidate
    dq n8_ippsECCPValidate
    dq y8_ippsECCPValidate
    dq e9_ippsECCPValidate
    dq l9_ippsECCPValidate
    dq n0_ippsECCPValidate
    dq k0_ippsECCPValidate
    dq k1_ippsECCPValidate

segment .text
global ippsECCPValidate:function (ippsECCPValidate.LEndippsECCPValidate - ippsECCPValidate)
.Lin_ippsECCPValidate:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPValidate:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPValidate]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPValidate:
