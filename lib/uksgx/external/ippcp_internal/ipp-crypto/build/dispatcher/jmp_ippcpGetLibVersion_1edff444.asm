%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippcpGetLibVersion%+elf_symbol_type
extern n8_ippcpGetLibVersion%+elf_symbol_type
extern y8_ippcpGetLibVersion%+elf_symbol_type
extern e9_ippcpGetLibVersion%+elf_symbol_type
extern l9_ippcpGetLibVersion%+elf_symbol_type
extern n0_ippcpGetLibVersion%+elf_symbol_type
extern k0_ippcpGetLibVersion%+elf_symbol_type
extern k1_ippcpGetLibVersion%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippcpGetLibVersion
.Larraddr_ippcpGetLibVersion:
    dq m7_ippcpGetLibVersion
    dq n8_ippcpGetLibVersion
    dq y8_ippcpGetLibVersion
    dq e9_ippcpGetLibVersion
    dq l9_ippcpGetLibVersion
    dq n0_ippcpGetLibVersion
    dq k0_ippcpGetLibVersion
    dq k1_ippcpGetLibVersion

segment .text
global ippcpGetLibVersion:function (ippcpGetLibVersion.LEndippcpGetLibVersion - ippcpGetLibVersion)
.Lin_ippcpGetLibVersion:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippcpGetLibVersion:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippcpGetLibVersion]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippcpGetLibVersion:
