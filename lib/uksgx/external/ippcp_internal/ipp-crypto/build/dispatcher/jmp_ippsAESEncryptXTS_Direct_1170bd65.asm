%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern n8_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern y8_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern e9_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern l9_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern n0_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern k0_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern k1_ippsAESEncryptXTS_Direct%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAESEncryptXTS_Direct
.Larraddr_ippsAESEncryptXTS_Direct:
    dq m7_ippsAESEncryptXTS_Direct
    dq n8_ippsAESEncryptXTS_Direct
    dq y8_ippsAESEncryptXTS_Direct
    dq e9_ippsAESEncryptXTS_Direct
    dq l9_ippsAESEncryptXTS_Direct
    dq n0_ippsAESEncryptXTS_Direct
    dq k0_ippsAESEncryptXTS_Direct
    dq k1_ippsAESEncryptXTS_Direct

segment .text
global ippsAESEncryptXTS_Direct:function (ippsAESEncryptXTS_Direct.LEndippsAESEncryptXTS_Direct - ippsAESEncryptXTS_Direct)
.Lin_ippsAESEncryptXTS_Direct:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAESEncryptXTS_Direct:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAESEncryptXTS_Direct]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAESEncryptXTS_Direct:
