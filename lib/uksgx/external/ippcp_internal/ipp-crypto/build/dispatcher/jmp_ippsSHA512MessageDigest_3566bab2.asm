%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA512MessageDigest%+elf_symbol_type
extern n8_ippsSHA512MessageDigest%+elf_symbol_type
extern y8_ippsSHA512MessageDigest%+elf_symbol_type
extern e9_ippsSHA512MessageDigest%+elf_symbol_type
extern l9_ippsSHA512MessageDigest%+elf_symbol_type
extern n0_ippsSHA512MessageDigest%+elf_symbol_type
extern k0_ippsSHA512MessageDigest%+elf_symbol_type
extern k1_ippsSHA512MessageDigest%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA512MessageDigest
.Larraddr_ippsSHA512MessageDigest:
    dq m7_ippsSHA512MessageDigest
    dq n8_ippsSHA512MessageDigest
    dq y8_ippsSHA512MessageDigest
    dq e9_ippsSHA512MessageDigest
    dq l9_ippsSHA512MessageDigest
    dq n0_ippsSHA512MessageDigest
    dq k0_ippsSHA512MessageDigest
    dq k1_ippsSHA512MessageDigest

segment .text
global ippsSHA512MessageDigest:function (ippsSHA512MessageDigest.LEndippsSHA512MessageDigest - ippsSHA512MessageDigest)
.Lin_ippsSHA512MessageDigest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA512MessageDigest:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA512MessageDigest]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA512MessageDigest:
