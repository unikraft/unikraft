%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsDLPGet%+elf_symbol_type
extern n8_ippsDLPGet%+elf_symbol_type
extern y8_ippsDLPGet%+elf_symbol_type
extern e9_ippsDLPGet%+elf_symbol_type
extern l9_ippsDLPGet%+elf_symbol_type
extern n0_ippsDLPGet%+elf_symbol_type
extern k0_ippsDLPGet%+elf_symbol_type
extern k1_ippsDLPGet%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsDLPGet
.Larraddr_ippsDLPGet:
    dq m7_ippsDLPGet
    dq n8_ippsDLPGet
    dq y8_ippsDLPGet
    dq e9_ippsDLPGet
    dq l9_ippsDLPGet
    dq n0_ippsDLPGet
    dq k0_ippsDLPGet
    dq k1_ippsDLPGet

segment .text
global ippsDLPGet:function (ippsDLPGet.LEndippsDLPGet - ippsDLPGet)
.Lin_ippsDLPGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsDLPGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsDLPGet]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsDLPGet:
