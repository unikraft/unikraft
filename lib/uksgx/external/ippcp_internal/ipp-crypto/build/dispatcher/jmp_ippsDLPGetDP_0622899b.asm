%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPGetDP%+elf_symbol_type
extern n8_ippsDLPGetDP%+elf_symbol_type
extern y8_ippsDLPGetDP%+elf_symbol_type
extern e9_ippsDLPGetDP%+elf_symbol_type
extern l9_ippsDLPGetDP%+elf_symbol_type
extern n0_ippsDLPGetDP%+elf_symbol_type
extern k0_ippsDLPGetDP%+elf_symbol_type
extern k1_ippsDLPGetDP%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPGetDP
.Larraddr_ippsDLPGetDP:
    dq m7_ippsDLPGetDP
    dq n8_ippsDLPGetDP
    dq y8_ippsDLPGetDP
    dq e9_ippsDLPGetDP
    dq l9_ippsDLPGetDP
    dq n0_ippsDLPGetDP
    dq k0_ippsDLPGetDP
    dq k1_ippsDLPGetDP

segment .text
global ippsDLPGetDP:function (ippsDLPGetDP.LEndippsDLPGetDP - ippsDLPGetDP)
.Lin_ippsDLPGetDP:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPGetDP:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPGetDP]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPGetDP:
