%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern n8_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern y8_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern e9_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern l9_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern n0_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern k0_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern k1_ippsSMS4DecryptCBC_CS2%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSMS4DecryptCBC_CS2
.Larraddr_ippsSMS4DecryptCBC_CS2:
    dq m7_ippsSMS4DecryptCBC_CS2
    dq n8_ippsSMS4DecryptCBC_CS2
    dq y8_ippsSMS4DecryptCBC_CS2
    dq e9_ippsSMS4DecryptCBC_CS2
    dq l9_ippsSMS4DecryptCBC_CS2
    dq n0_ippsSMS4DecryptCBC_CS2
    dq k0_ippsSMS4DecryptCBC_CS2
    dq k1_ippsSMS4DecryptCBC_CS2

segment .text
global ippsSMS4DecryptCBC_CS2:function (ippsSMS4DecryptCBC_CS2.LEndippsSMS4DecryptCBC_CS2 - ippsSMS4DecryptCBC_CS2)
.Lin_ippsSMS4DecryptCBC_CS2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSMS4DecryptCBC_CS2:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSMS4DecryptCBC_CS2]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSMS4DecryptCBC_CS2:
