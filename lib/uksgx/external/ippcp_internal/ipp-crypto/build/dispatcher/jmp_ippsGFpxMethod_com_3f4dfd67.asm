%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpxMethod_com%+elf_symbol_type
extern n8_ippsGFpxMethod_com%+elf_symbol_type
extern y8_ippsGFpxMethod_com%+elf_symbol_type
extern e9_ippsGFpxMethod_com%+elf_symbol_type
extern l9_ippsGFpxMethod_com%+elf_symbol_type
extern n0_ippsGFpxMethod_com%+elf_symbol_type
extern k0_ippsGFpxMethod_com%+elf_symbol_type
extern k1_ippsGFpxMethod_com%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpxMethod_com
.Larraddr_ippsGFpxMethod_com:
    dq m7_ippsGFpxMethod_com
    dq n8_ippsGFpxMethod_com
    dq y8_ippsGFpxMethod_com
    dq e9_ippsGFpxMethod_com
    dq l9_ippsGFpxMethod_com
    dq n0_ippsGFpxMethod_com
    dq k0_ippsGFpxMethod_com
    dq k1_ippsGFpxMethod_com

segment .text
global ippsGFpxMethod_com:function (ippsGFpxMethod_com.LEndippsGFpxMethod_com - ippsGFpxMethod_com)
.Lin_ippsGFpxMethod_com:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpxMethod_com:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpxMethod_com]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpxMethod_com:
