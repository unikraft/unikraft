%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpAdd_PE%+elf_symbol_type
extern n8_ippsGFpAdd_PE%+elf_symbol_type
extern y8_ippsGFpAdd_PE%+elf_symbol_type
extern e9_ippsGFpAdd_PE%+elf_symbol_type
extern l9_ippsGFpAdd_PE%+elf_symbol_type
extern n0_ippsGFpAdd_PE%+elf_symbol_type
extern k0_ippsGFpAdd_PE%+elf_symbol_type
extern k1_ippsGFpAdd_PE%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpAdd_PE
.Larraddr_ippsGFpAdd_PE:
    dq m7_ippsGFpAdd_PE
    dq n8_ippsGFpAdd_PE
    dq y8_ippsGFpAdd_PE
    dq e9_ippsGFpAdd_PE
    dq l9_ippsGFpAdd_PE
    dq n0_ippsGFpAdd_PE
    dq k0_ippsGFpAdd_PE
    dq k1_ippsGFpAdd_PE

segment .text
global ippsGFpAdd_PE:function (ippsGFpAdd_PE.LEndippsGFpAdd_PE - ippsGFpAdd_PE)
.Lin_ippsGFpAdd_PE:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpAdd_PE:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpAdd_PE]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpAdd_PE:
