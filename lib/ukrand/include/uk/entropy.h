#ifndef __ENTROPY__
#define __ENTROPY__

#include <uk/plat/lcpu.h>

struct fast_pool {
    __u32 pool[4];
    __u32 last;
    __u16 reg_index;
    __u8 count;
};

void add_interrupt_randomness(int irq);

static inline void get_registers(struct __regs *regs) {
    asm(
        "mov %%r15, %0 \n\t\
         mov %%r14, %1 \n\t\
         mov %%r13, %2 \n\t\
         mov %%r12, %3 \n\t\
         mov %%r11, %4 \n\t\
         mov %%r10, %5 \n\t\
         mov %%r9, %6 \n\t\
         mov %%r8, %7 \n\t\
         mov %%rbp, %8 \n\t\
         mov %%rbx, %9 \n\t\
         mov %%rax, %10 \n\t\
         mov %%rcx, %11 \n\t\
         mov %%rdx, %12 \n\t\
         mov %%rsi, %13 \n\t\
         mov %%rdi, %14 \n\t\
         mov %%cs, %15 \n\t\
         mov %%rsp, %16 \n\t\
         mov %%ss, %17 \n\t\
        "
        
        : "=rm" ( regs->r15 ),
          "=rm" ( regs->r14 ),
          "=rm" ( regs->r13 ),
          "=rm" ( regs->r12 ),
          "=rm" ( regs->r11 ),
          "=rm" ( regs->r10 ),
          "=rm" ( regs->r9 ),
          "=rm" ( regs->r8 ),
          "=rm" ( regs->rbp ),
          "=rm" ( regs->rbx ),
          "=rm" ( regs->rax ),
          "=rm" ( regs->rcx ),
          "=rm" ( regs->rdx ),
          "=rm" ( regs->rsi ),
          "=rm" ( regs->rdi ),
          "=rm" ( regs->cs ),
          "=rm" ( regs->rsp ),
          "=rm" ( regs->ss )
    );
}

static inline uint64_t get_rip() {
    __u64 rip;

    asm(
        "call next \n\t\
         next: pop %%rax \n\t\
         mov %%rax, %0 \n\t\
        "
         : "=rm"(rip)
    );

    return rip;
}
#endif