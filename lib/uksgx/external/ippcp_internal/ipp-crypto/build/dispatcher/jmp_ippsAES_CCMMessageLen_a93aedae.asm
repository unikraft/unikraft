%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsAES_CCMMessageLen%+elf_symbol_type
extern n8_ippsAES_CCMMessageLen%+elf_symbol_type
extern y8_ippsAES_CCMMessageLen%+elf_symbol_type
extern e9_ippsAES_CCMMessageLen%+elf_symbol_type
extern l9_ippsAES_CCMMessageLen%+elf_symbol_type
extern n0_ippsAES_CCMMessageLen%+elf_symbol_type
extern k0_ippsAES_CCMMessageLen%+elf_symbol_type
extern k1_ippsAES_CCMMessageLen%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsAES_CCMMessageLen
.Larraddr_ippsAES_CCMMessageLen:
    dq m7_ippsAES_CCMMessageLen
    dq n8_ippsAES_CCMMessageLen
    dq y8_ippsAES_CCMMessageLen
    dq e9_ippsAES_CCMMessageLen
    dq l9_ippsAES_CCMMessageLen
    dq n0_ippsAES_CCMMessageLen
    dq k0_ippsAES_CCMMessageLen
    dq k1_ippsAES_CCMMessageLen

segment .text
global ippsAES_CCMMessageLen:function (ippsAES_CCMMessageLen.LEndippsAES_CCMMessageLen - ippsAES_CCMMessageLen)
.Lin_ippsAES_CCMMessageLen:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsAES_CCMMessageLen:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsAES_CCMMessageLen]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsAES_CCMMessageLen:
