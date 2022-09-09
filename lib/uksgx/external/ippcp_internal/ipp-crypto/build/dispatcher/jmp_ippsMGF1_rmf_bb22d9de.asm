%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMGF1_rmf%+elf_symbol_type
extern n8_ippsMGF1_rmf%+elf_symbol_type
extern y8_ippsMGF1_rmf%+elf_symbol_type
extern e9_ippsMGF1_rmf%+elf_symbol_type
extern l9_ippsMGF1_rmf%+elf_symbol_type
extern n0_ippsMGF1_rmf%+elf_symbol_type
extern k0_ippsMGF1_rmf%+elf_symbol_type
extern k1_ippsMGF1_rmf%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMGF1_rmf
.Larraddr_ippsMGF1_rmf:
    dq m7_ippsMGF1_rmf
    dq n8_ippsMGF1_rmf
    dq y8_ippsMGF1_rmf
    dq e9_ippsMGF1_rmf
    dq l9_ippsMGF1_rmf
    dq n0_ippsMGF1_rmf
    dq k0_ippsMGF1_rmf
    dq k1_ippsMGF1_rmf

segment .text
global ippsMGF1_rmf:function (ippsMGF1_rmf.LEndippsMGF1_rmf - ippsMGF1_rmf)
.Lin_ippsMGF1_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMGF1_rmf:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMGF1_rmf]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMGF1_rmf:
