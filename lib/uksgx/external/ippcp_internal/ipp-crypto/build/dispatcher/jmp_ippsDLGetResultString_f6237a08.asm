%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLGetResultString%+elf_symbol_type
extern n8_ippsDLGetResultString%+elf_symbol_type
extern y8_ippsDLGetResultString%+elf_symbol_type
extern e9_ippsDLGetResultString%+elf_symbol_type
extern l9_ippsDLGetResultString%+elf_symbol_type
extern n0_ippsDLGetResultString%+elf_symbol_type
extern k0_ippsDLGetResultString%+elf_symbol_type
extern k1_ippsDLGetResultString%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLGetResultString
.Larraddr_ippsDLGetResultString:
    dq m7_ippsDLGetResultString
    dq n8_ippsDLGetResultString
    dq y8_ippsDLGetResultString
    dq e9_ippsDLGetResultString
    dq l9_ippsDLGetResultString
    dq n0_ippsDLGetResultString
    dq k0_ippsDLGetResultString
    dq k1_ippsDLGetResultString

segment .text
global ippsDLGetResultString:function (ippsDLGetResultString.LEndippsDLGetResultString - ippsDLGetResultString)
.Lin_ippsDLGetResultString:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLGetResultString:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLGetResultString]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLGetResultString:
