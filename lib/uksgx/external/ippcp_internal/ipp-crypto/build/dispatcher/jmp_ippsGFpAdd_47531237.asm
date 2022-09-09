%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpAdd%+elf_symbol_type
extern n8_ippsGFpAdd%+elf_symbol_type
extern y8_ippsGFpAdd%+elf_symbol_type
extern e9_ippsGFpAdd%+elf_symbol_type
extern l9_ippsGFpAdd%+elf_symbol_type
extern n0_ippsGFpAdd%+elf_symbol_type
extern k0_ippsGFpAdd%+elf_symbol_type
extern k1_ippsGFpAdd%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpAdd
.Larraddr_ippsGFpAdd:
    dq m7_ippsGFpAdd
    dq n8_ippsGFpAdd
    dq y8_ippsGFpAdd
    dq e9_ippsGFpAdd
    dq l9_ippsGFpAdd
    dq n0_ippsGFpAdd
    dq k0_ippsGFpAdd
    dq k1_ippsGFpAdd

segment .text
global ippsGFpAdd:function (ippsGFpAdd.LEndippsGFpAdd - ippsGFpAdd)
.Lin_ippsGFpAdd:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpAdd:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpAdd]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpAdd:
