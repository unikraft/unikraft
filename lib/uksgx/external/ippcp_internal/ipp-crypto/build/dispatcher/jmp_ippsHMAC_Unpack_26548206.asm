%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsHMAC_Unpack%+elf_symbol_type
extern n8_ippsHMAC_Unpack%+elf_symbol_type
extern y8_ippsHMAC_Unpack%+elf_symbol_type
extern e9_ippsHMAC_Unpack%+elf_symbol_type
extern l9_ippsHMAC_Unpack%+elf_symbol_type
extern n0_ippsHMAC_Unpack%+elf_symbol_type
extern k0_ippsHMAC_Unpack%+elf_symbol_type
extern k1_ippsHMAC_Unpack%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsHMAC_Unpack
.Larraddr_ippsHMAC_Unpack:
    dq m7_ippsHMAC_Unpack
    dq n8_ippsHMAC_Unpack
    dq y8_ippsHMAC_Unpack
    dq e9_ippsHMAC_Unpack
    dq l9_ippsHMAC_Unpack
    dq n0_ippsHMAC_Unpack
    dq k0_ippsHMAC_Unpack
    dq k1_ippsHMAC_Unpack

segment .text
global ippsHMAC_Unpack:function (ippsHMAC_Unpack.LEndippsHMAC_Unpack - ippsHMAC_Unpack)
.Lin_ippsHMAC_Unpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsHMAC_Unpack:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsHMAC_Unpack]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsHMAC_Unpack:
