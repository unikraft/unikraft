%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA256GetSize%+elf_symbol_type
extern n8_ippsSHA256GetSize%+elf_symbol_type
extern y8_ippsSHA256GetSize%+elf_symbol_type
extern e9_ippsSHA256GetSize%+elf_symbol_type
extern l9_ippsSHA256GetSize%+elf_symbol_type
extern n0_ippsSHA256GetSize%+elf_symbol_type
extern k0_ippsSHA256GetSize%+elf_symbol_type
extern k1_ippsSHA256GetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA256GetSize
.Larraddr_ippsSHA256GetSize:
    dq m7_ippsSHA256GetSize
    dq n8_ippsSHA256GetSize
    dq y8_ippsSHA256GetSize
    dq e9_ippsSHA256GetSize
    dq l9_ippsSHA256GetSize
    dq n0_ippsSHA256GetSize
    dq k0_ippsSHA256GetSize
    dq k1_ippsSHA256GetSize

segment .text
global ippsSHA256GetSize:function (ippsSHA256GetSize.LEndippsSHA256GetSize - ippsSHA256GetSize)
.Lin_ippsSHA256GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA256GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA256GetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA256GetSize:
