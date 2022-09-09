%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA1GetSize%+elf_symbol_type
extern n8_ippsSHA1GetSize%+elf_symbol_type
extern y8_ippsSHA1GetSize%+elf_symbol_type
extern e9_ippsSHA1GetSize%+elf_symbol_type
extern l9_ippsSHA1GetSize%+elf_symbol_type
extern n0_ippsSHA1GetSize%+elf_symbol_type
extern k0_ippsSHA1GetSize%+elf_symbol_type
extern k1_ippsSHA1GetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA1GetSize
.Larraddr_ippsSHA1GetSize:
    dq m7_ippsSHA1GetSize
    dq n8_ippsSHA1GetSize
    dq y8_ippsSHA1GetSize
    dq e9_ippsSHA1GetSize
    dq l9_ippsSHA1GetSize
    dq n0_ippsSHA1GetSize
    dq k0_ippsSHA1GetSize
    dq k1_ippsSHA1GetSize

segment .text
global ippsSHA1GetSize:function (ippsSHA1GetSize.LEndippsSHA1GetSize - ippsSHA1GetSize)
.Lin_ippsSHA1GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA1GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA1GetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA1GetSize:
