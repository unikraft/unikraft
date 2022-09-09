%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA512GetSize%+elf_symbol_type
extern n8_ippsSHA512GetSize%+elf_symbol_type
extern y8_ippsSHA512GetSize%+elf_symbol_type
extern e9_ippsSHA512GetSize%+elf_symbol_type
extern l9_ippsSHA512GetSize%+elf_symbol_type
extern n0_ippsSHA512GetSize%+elf_symbol_type
extern k0_ippsSHA512GetSize%+elf_symbol_type
extern k1_ippsSHA512GetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA512GetSize
.Larraddr_ippsSHA512GetSize:
    dq m7_ippsSHA512GetSize
    dq n8_ippsSHA512GetSize
    dq y8_ippsSHA512GetSize
    dq e9_ippsSHA512GetSize
    dq l9_ippsSHA512GetSize
    dq n0_ippsSHA512GetSize
    dq k0_ippsSHA512GetSize
    dq k1_ippsSHA512GetSize

segment .text
global ippsSHA512GetSize:function (ippsSHA512GetSize.LEndippsSHA512GetSize - ippsSHA512GetSize)
.Lin_ippsSHA512GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA512GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA512GetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA512GetSize:
