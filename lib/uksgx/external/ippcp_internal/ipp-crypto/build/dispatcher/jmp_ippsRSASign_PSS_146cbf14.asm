%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsRSASign_PSS%+elf_symbol_type
extern n8_ippsRSASign_PSS%+elf_symbol_type
extern y8_ippsRSASign_PSS%+elf_symbol_type
extern e9_ippsRSASign_PSS%+elf_symbol_type
extern l9_ippsRSASign_PSS%+elf_symbol_type
extern n0_ippsRSASign_PSS%+elf_symbol_type
extern k0_ippsRSASign_PSS%+elf_symbol_type
extern k1_ippsRSASign_PSS%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsRSASign_PSS
.Larraddr_ippsRSASign_PSS:
    dq m7_ippsRSASign_PSS
    dq n8_ippsRSASign_PSS
    dq y8_ippsRSASign_PSS
    dq e9_ippsRSASign_PSS
    dq l9_ippsRSASign_PSS
    dq n0_ippsRSASign_PSS
    dq k0_ippsRSASign_PSS
    dq k1_ippsRSASign_PSS

segment .text
global ippsRSASign_PSS:function (ippsRSASign_PSS.LEndippsRSASign_PSS - ippsRSASign_PSS)
.Lin_ippsRSASign_PSS:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsRSASign_PSS:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsRSASign_PSS]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsRSASign_PSS:
