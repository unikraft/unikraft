%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSM3MessageDigest%+elf_symbol_type
extern n8_ippsSM3MessageDigest%+elf_symbol_type
extern y8_ippsSM3MessageDigest%+elf_symbol_type
extern e9_ippsSM3MessageDigest%+elf_symbol_type
extern l9_ippsSM3MessageDigest%+elf_symbol_type
extern n0_ippsSM3MessageDigest%+elf_symbol_type
extern k0_ippsSM3MessageDigest%+elf_symbol_type
extern k1_ippsSM3MessageDigest%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSM3MessageDigest
.Larraddr_ippsSM3MessageDigest:
    dq m7_ippsSM3MessageDigest
    dq n8_ippsSM3MessageDigest
    dq y8_ippsSM3MessageDigest
    dq e9_ippsSM3MessageDigest
    dq l9_ippsSM3MessageDigest
    dq n0_ippsSM3MessageDigest
    dq k0_ippsSM3MessageDigest
    dq k1_ippsSM3MessageDigest

segment .text
global ippsSM3MessageDigest:function (ippsSM3MessageDigest.LEndippsSM3MessageDigest - ippsSM3MessageDigest)
.Lin_ippsSM3MessageDigest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSM3MessageDigest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSM3MessageDigest]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSM3MessageDigest:
