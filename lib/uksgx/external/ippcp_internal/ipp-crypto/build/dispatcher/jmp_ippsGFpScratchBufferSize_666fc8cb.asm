%if ((__NASM_MAJOR__ > 2) || ((__NASM_MAJOR__ == 2) && (__NASM_MINOR__ > 14)))
  %xdefine elf_symbol_type :function
%else
  %xdefine elf_symbol_type
%endif
extern m7_ippsGFpScratchBufferSize%+elf_symbol_type
extern n8_ippsGFpScratchBufferSize%+elf_symbol_type
extern y8_ippsGFpScratchBufferSize%+elf_symbol_type
extern e9_ippsGFpScratchBufferSize%+elf_symbol_type
extern l9_ippsGFpScratchBufferSize%+elf_symbol_type
extern n0_ippsGFpScratchBufferSize%+elf_symbol_type
extern k0_ippsGFpScratchBufferSize%+elf_symbol_type
extern k1_ippsGFpScratchBufferSize%+elf_symbol_type
extern ippcpJumpIndexForMergedLibs
extern ippcpSafeInit%+elf_symbol_type


segment .data
align 8
dq  .Lin_ippsGFpScratchBufferSize
.Larraddr_ippsGFpScratchBufferSize:
    dq m7_ippsGFpScratchBufferSize
    dq n8_ippsGFpScratchBufferSize
    dq y8_ippsGFpScratchBufferSize
    dq e9_ippsGFpScratchBufferSize
    dq l9_ippsGFpScratchBufferSize
    dq n0_ippsGFpScratchBufferSize
    dq k0_ippsGFpScratchBufferSize
    dq k1_ippsGFpScratchBufferSize

segment .text
global ippsGFpScratchBufferSize:function (ippsGFpScratchBufferSize.LEndippsGFpScratchBufferSize - ippsGFpScratchBufferSize)
.Lin_ippsGFpScratchBufferSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    call ippcpSafeInit wrt ..plt
    align 16

ippsGFpScratchBufferSize:
    db 0xf3, 0x0f, 0x1e, 0xfa
    mov     rax, qword [rel ippcpJumpIndexForMergedLibs wrt ..gotpc]
    movsxd  rax, dword [rax]
    lea     r11, [rel .Larraddr_ippsGFpScratchBufferSize]
    mov     r11, qword [r11+rax*8]
    jmp     r11
.LEndippsGFpScratchBufferSize:
