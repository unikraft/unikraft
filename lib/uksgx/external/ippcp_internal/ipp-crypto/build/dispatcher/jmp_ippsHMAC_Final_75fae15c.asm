%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMAC_Final%+elf_symbol_type
extern n8_ippsHMAC_Final%+elf_symbol_type
extern y8_ippsHMAC_Final%+elf_symbol_type
extern e9_ippsHMAC_Final%+elf_symbol_type
extern l9_ippsHMAC_Final%+elf_symbol_type
extern n0_ippsHMAC_Final%+elf_symbol_type
extern k0_ippsHMAC_Final%+elf_symbol_type
extern k1_ippsHMAC_Final%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMAC_Final
.Larraddr_ippsHMAC_Final:
    dq m7_ippsHMAC_Final
    dq n8_ippsHMAC_Final
    dq y8_ippsHMAC_Final
    dq e9_ippsHMAC_Final
    dq l9_ippsHMAC_Final
    dq n0_ippsHMAC_Final
    dq k0_ippsHMAC_Final
    dq k1_ippsHMAC_Final

segment .text
global ippsHMAC_Final:function (ippsHMAC_Final.LEndippsHMAC_Final - ippsHMAC_Final)
.Lin_ippsHMAC_Final:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMAC_Final:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMAC_Final]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMAC_Final:
