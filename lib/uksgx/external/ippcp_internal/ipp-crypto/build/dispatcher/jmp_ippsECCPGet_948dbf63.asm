%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsECCPGet%+elf_symbol_type
extern n8_ippsECCPGet%+elf_symbol_type
extern y8_ippsECCPGet%+elf_symbol_type
extern e9_ippsECCPGet%+elf_symbol_type
extern l9_ippsECCPGet%+elf_symbol_type
extern n0_ippsECCPGet%+elf_symbol_type
extern k0_ippsECCPGet%+elf_symbol_type
extern k1_ippsECCPGet%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsECCPGet
.Larraddr_ippsECCPGet:
    dq m7_ippsECCPGet
    dq n8_ippsECCPGet
    dq y8_ippsECCPGet
    dq e9_ippsECCPGet
    dq l9_ippsECCPGet
    dq n0_ippsECCPGet
    dq k0_ippsECCPGet
    dq k1_ippsECCPGet

segment .text
global ippsECCPGet:function (ippsECCPGet.LEndippsECCPGet - ippsECCPGet)
.Lin_ippsECCPGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsECCPGet:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsECCPGet]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsECCPGet:
