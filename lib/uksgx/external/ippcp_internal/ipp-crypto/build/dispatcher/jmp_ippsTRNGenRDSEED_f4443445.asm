%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsTRNGenRDSEED%+elf_symbol_type
extern n8_ippsTRNGenRDSEED%+elf_symbol_type
extern y8_ippsTRNGenRDSEED%+elf_symbol_type
extern e9_ippsTRNGenRDSEED%+elf_symbol_type
extern l9_ippsTRNGenRDSEED%+elf_symbol_type
extern n0_ippsTRNGenRDSEED%+elf_symbol_type
extern k0_ippsTRNGenRDSEED%+elf_symbol_type
extern k1_ippsTRNGenRDSEED%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsTRNGenRDSEED
.Larraddr_ippsTRNGenRDSEED:
    dq m7_ippsTRNGenRDSEED
    dq n8_ippsTRNGenRDSEED
    dq y8_ippsTRNGenRDSEED
    dq e9_ippsTRNGenRDSEED
    dq l9_ippsTRNGenRDSEED
    dq n0_ippsTRNGenRDSEED
    dq k0_ippsTRNGenRDSEED
    dq k1_ippsTRNGenRDSEED

segment .text
global ippsTRNGenRDSEED:function (ippsTRNGenRDSEED.LEndippsTRNGenRDSEED - ippsTRNGenRDSEED)
.Lin_ippsTRNGenRDSEED:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsTRNGenRDSEED:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsTRNGenRDSEED]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsTRNGenRDSEED:
