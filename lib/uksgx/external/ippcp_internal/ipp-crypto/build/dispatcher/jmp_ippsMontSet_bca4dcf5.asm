%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsMontSet%+elf_symbol_type
extern n8_ippsMontSet%+elf_symbol_type
extern y8_ippsMontSet%+elf_symbol_type
extern e9_ippsMontSet%+elf_symbol_type
extern l9_ippsMontSet%+elf_symbol_type
extern n0_ippsMontSet%+elf_symbol_type
extern k0_ippsMontSet%+elf_symbol_type
extern k1_ippsMontSet%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsMontSet
.Larraddr_ippsMontSet:
    dq m7_ippsMontSet
    dq n8_ippsMontSet
    dq y8_ippsMontSet
    dq e9_ippsMontSet
    dq l9_ippsMontSet
    dq n0_ippsMontSet
    dq k0_ippsMontSet
    dq k1_ippsMontSet

segment .text
global ippsMontSet:function (ippsMontSet.LEndippsMontSet - ippsMontSet)
.Lin_ippsMontSet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsMontSet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsMontSet]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsMontSet:
