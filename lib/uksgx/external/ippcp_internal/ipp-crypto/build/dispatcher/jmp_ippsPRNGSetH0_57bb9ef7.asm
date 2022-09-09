%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPRNGSetH0%+elf_symbol_type
extern n8_ippsPRNGSetH0%+elf_symbol_type
extern y8_ippsPRNGSetH0%+elf_symbol_type
extern e9_ippsPRNGSetH0%+elf_symbol_type
extern l9_ippsPRNGSetH0%+elf_symbol_type
extern n0_ippsPRNGSetH0%+elf_symbol_type
extern k0_ippsPRNGSetH0%+elf_symbol_type
extern k1_ippsPRNGSetH0%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPRNGSetH0
.Larraddr_ippsPRNGSetH0:
    dq m7_ippsPRNGSetH0
    dq n8_ippsPRNGSetH0
    dq y8_ippsPRNGSetH0
    dq e9_ippsPRNGSetH0
    dq l9_ippsPRNGSetH0
    dq n0_ippsPRNGSetH0
    dq k0_ippsPRNGSetH0
    dq k1_ippsPRNGSetH0

segment .text
global ippsPRNGSetH0:function (ippsPRNGSetH0.LEndippsPRNGSetH0 - ippsPRNGSetH0)
.Lin_ippsPRNGSetH0:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPRNGSetH0:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPRNGSetH0]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPRNGSetH0:
