%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsPRNGenRDRAND%+elf_symbol_type
extern n8_ippsPRNGenRDRAND%+elf_symbol_type
extern y8_ippsPRNGenRDRAND%+elf_symbol_type
extern e9_ippsPRNGenRDRAND%+elf_symbol_type
extern l9_ippsPRNGenRDRAND%+elf_symbol_type
extern n0_ippsPRNGenRDRAND%+elf_symbol_type
extern k0_ippsPRNGenRDRAND%+elf_symbol_type
extern k1_ippsPRNGenRDRAND%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsPRNGenRDRAND
.Larraddr_ippsPRNGenRDRAND:
    dq m7_ippsPRNGenRDRAND
    dq n8_ippsPRNGenRDRAND
    dq y8_ippsPRNGenRDRAND
    dq e9_ippsPRNGenRDRAND
    dq l9_ippsPRNGenRDRAND
    dq n0_ippsPRNGenRDRAND
    dq k0_ippsPRNGenRDRAND
    dq k1_ippsPRNGenRDRAND

segment .text
global ippsPRNGenRDRAND:function (ippsPRNGenRDRAND.LEndippsPRNGenRDRAND - ippsPRNGenRDRAND)
.Lin_ippsPRNGenRDRAND:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsPRNGenRDRAND:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsPRNGenRDRAND]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsPRNGenRDRAND:
