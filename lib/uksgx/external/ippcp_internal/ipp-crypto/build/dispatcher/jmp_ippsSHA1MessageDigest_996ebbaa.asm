%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA1MessageDigest%+elf_symbol_type
extern n8_ippsSHA1MessageDigest%+elf_symbol_type
extern y8_ippsSHA1MessageDigest%+elf_symbol_type
extern e9_ippsSHA1MessageDigest%+elf_symbol_type
extern l9_ippsSHA1MessageDigest%+elf_symbol_type
extern n0_ippsSHA1MessageDigest%+elf_symbol_type
extern k0_ippsSHA1MessageDigest%+elf_symbol_type
extern k1_ippsSHA1MessageDigest%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA1MessageDigest
.Larraddr_ippsSHA1MessageDigest:
    dq m7_ippsSHA1MessageDigest
    dq n8_ippsSHA1MessageDigest
    dq y8_ippsSHA1MessageDigest
    dq e9_ippsSHA1MessageDigest
    dq l9_ippsSHA1MessageDigest
    dq n0_ippsSHA1MessageDigest
    dq k0_ippsSHA1MessageDigest
    dq k1_ippsSHA1MessageDigest

segment .text
global ippsSHA1MessageDigest:function (ippsSHA1MessageDigest.LEndippsSHA1MessageDigest - ippsSHA1MessageDigest)
.Lin_ippsSHA1MessageDigest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA1MessageDigest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA1MessageDigest]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA1MessageDigest:
