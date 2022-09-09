%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPSetDP%+elf_symbol_type
extern n8_ippsDLPSetDP%+elf_symbol_type
extern y8_ippsDLPSetDP%+elf_symbol_type
extern e9_ippsDLPSetDP%+elf_symbol_type
extern l9_ippsDLPSetDP%+elf_symbol_type
extern n0_ippsDLPSetDP%+elf_symbol_type
extern k0_ippsDLPSetDP%+elf_symbol_type
extern k1_ippsDLPSetDP%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPSetDP
.Larraddr_ippsDLPSetDP:
    dq m7_ippsDLPSetDP
    dq n8_ippsDLPSetDP
    dq y8_ippsDLPSetDP
    dq e9_ippsDLPSetDP
    dq l9_ippsDLPSetDP
    dq n0_ippsDLPSetDP
    dq k0_ippsDLPSetDP
    dq k1_ippsDLPSetDP

segment .text
global ippsDLPSetDP:function (ippsDLPSetDP.LEndippsDLPSetDP - ippsDLPSetDP)
.Lin_ippsDLPSetDP:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPSetDP:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPSetDP]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPSetDP:
