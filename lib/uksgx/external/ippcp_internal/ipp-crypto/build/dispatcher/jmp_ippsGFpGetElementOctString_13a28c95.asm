%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpGetElementOctString%+elf_symbol_type
extern n8_ippsGFpGetElementOctString%+elf_symbol_type
extern y8_ippsGFpGetElementOctString%+elf_symbol_type
extern e9_ippsGFpGetElementOctString%+elf_symbol_type
extern l9_ippsGFpGetElementOctString%+elf_symbol_type
extern n0_ippsGFpGetElementOctString%+elf_symbol_type
extern k0_ippsGFpGetElementOctString%+elf_symbol_type
extern k1_ippsGFpGetElementOctString%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpGetElementOctString
.Larraddr_ippsGFpGetElementOctString:
    dq m7_ippsGFpGetElementOctString
    dq n8_ippsGFpGetElementOctString
    dq y8_ippsGFpGetElementOctString
    dq e9_ippsGFpGetElementOctString
    dq l9_ippsGFpGetElementOctString
    dq n0_ippsGFpGetElementOctString
    dq k0_ippsGFpGetElementOctString
    dq k1_ippsGFpGetElementOctString

segment .text
global ippsGFpGetElementOctString:function (ippsGFpGetElementOctString.LEndippsGFpGetElementOctString - ippsGFpGetElementOctString)
.Lin_ippsGFpGetElementOctString:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpGetElementOctString:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpGetElementOctString]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpGetElementOctString:
