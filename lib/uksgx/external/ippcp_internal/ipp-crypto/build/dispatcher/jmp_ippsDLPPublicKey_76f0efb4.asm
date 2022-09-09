%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPPublicKey%+elf_symbol_type
extern n8_ippsDLPPublicKey%+elf_symbol_type
extern y8_ippsDLPPublicKey%+elf_symbol_type
extern e9_ippsDLPPublicKey%+elf_symbol_type
extern l9_ippsDLPPublicKey%+elf_symbol_type
extern n0_ippsDLPPublicKey%+elf_symbol_type
extern k0_ippsDLPPublicKey%+elf_symbol_type
extern k1_ippsDLPPublicKey%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPPublicKey
.Larraddr_ippsDLPPublicKey:
    dq m7_ippsDLPPublicKey
    dq n8_ippsDLPPublicKey
    dq y8_ippsDLPPublicKey
    dq e9_ippsDLPPublicKey
    dq l9_ippsDLPPublicKey
    dq n0_ippsDLPPublicKey
    dq k0_ippsDLPPublicKey
    dq k1_ippsDLPPublicKey

segment .text
global ippsDLPPublicKey:function (ippsDLPPublicKey.LEndippsDLPPublicKey - ippsDLPPublicKey)
.Lin_ippsDLPPublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPPublicKey:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPPublicKey]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPPublicKey:
