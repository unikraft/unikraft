%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpSetElementOctString%+elf_symbol_type
extern n8_ippsGFpSetElementOctString%+elf_symbol_type
extern y8_ippsGFpSetElementOctString%+elf_symbol_type
extern e9_ippsGFpSetElementOctString%+elf_symbol_type
extern l9_ippsGFpSetElementOctString%+elf_symbol_type
extern n0_ippsGFpSetElementOctString%+elf_symbol_type
extern k0_ippsGFpSetElementOctString%+elf_symbol_type
extern k1_ippsGFpSetElementOctString%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpSetElementOctString
.Larraddr_ippsGFpSetElementOctString:
    dq m7_ippsGFpSetElementOctString
    dq n8_ippsGFpSetElementOctString
    dq y8_ippsGFpSetElementOctString
    dq e9_ippsGFpSetElementOctString
    dq l9_ippsGFpSetElementOctString
    dq n0_ippsGFpSetElementOctString
    dq k0_ippsGFpSetElementOctString
    dq k1_ippsGFpSetElementOctString

segment .text
global ippsGFpSetElementOctString:function (ippsGFpSetElementOctString.LEndippsGFpSetElementOctString - ippsGFpSetElementOctString)
.Lin_ippsGFpSetElementOctString:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpSetElementOctString:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpSetElementOctString]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpSetElementOctString:
