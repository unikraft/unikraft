%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsSHA384GetSize%+elf_symbol_type
extern n8_ippsSHA384GetSize%+elf_symbol_type
extern y8_ippsSHA384GetSize%+elf_symbol_type
extern e9_ippsSHA384GetSize%+elf_symbol_type
extern l9_ippsSHA384GetSize%+elf_symbol_type
extern n0_ippsSHA384GetSize%+elf_symbol_type
extern k0_ippsSHA384GetSize%+elf_symbol_type
extern k1_ippsSHA384GetSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsSHA384GetSize
.Larraddr_ippsSHA384GetSize:
    dq m7_ippsSHA384GetSize
    dq n8_ippsSHA384GetSize
    dq y8_ippsSHA384GetSize
    dq e9_ippsSHA384GetSize
    dq l9_ippsSHA384GetSize
    dq n0_ippsSHA384GetSize
    dq k0_ippsSHA384GetSize
    dq k1_ippsSHA384GetSize

segment .text
global ippsSHA384GetSize:function (ippsSHA384GetSize.LEndippsSHA384GetSize - ippsSHA384GetSize)
.Lin_ippsSHA384GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsSHA384GetSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsSHA384GetSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsSHA384GetSize:
