#include <uk/entropy.h>
#include <uk/print.h>
#include <uk/plat/time.h>
#include <uk/bitops.h>

static struct fast_pool fast_pool = {
    .reg_index = 0,
};

/*
 * This is a fast mixing routine used by the interrupt randomness
 * collector.  It's hardcoded for an 128 bit pool and assumes that any
 * locks that might be needed are taken by the caller.
 */
static void fast_mix(struct fast_pool *f)
{
	__u32 a = f->pool[0],	b = f->pool[1];
	__u32 c = f->pool[2],	d = f->pool[3];

	a += b;			c += d;
	b = uk_rol32(b, 6);	d = uk_rol32(d, 27);
	d ^= a;			b ^= c;

	a += b;			c += d;
	b = uk_rol32(b, 16);	d = uk_rol32(d, 14);
	d ^= a;			b ^= c;

	a += b;			c += d;
	b = uk_rol32(b, 6);	d = uk_rol32(d, 27);
	d ^= a;			b ^= c;

	a += b;			c += d;
	b = uk_rol32(b, 16);	d = uk_rol32(d, 14);
	d ^= a;			b ^= c;

	f->pool[0] = a;  f->pool[1] = b;
	f->pool[2] = c;  f->pool[3] = d;
	f->count++;
}

static __u64 get_reg(struct fast_pool *f_pool, struct __regs *regs) {
    __u64 *ptr = (__u64 *) regs;
    __u16 index;

    if (regs == NULL) {
        return 0;
    }

    /*
     * jmp over padding and rip register
    */
    index = f_pool->reg_index;
    if (index == 0 || 
        index == (__u64 *)regs->rip - (__u64 *)regs) {
        index++;
    }

    if (index >= sizeof(struct __regs) / sizeof(__u64)) {
        index = 0;
    }

    ptr += index++;
    f_pool->reg_index = index;

    return *ptr;

}

void add_interrupt_randomness(int irq) {
    struct __regs regs;
    __u64 startup_time_ns = ukplat_monotonic_clock();
    __u64 rip = get_rip();
    __u64 reg;
    __u32 reg_high, startup_high;

    get_registers(&regs);
    reg = get_reg(&fast_pool, &regs);

    reg_high = reg >> 32;
    startup_high = reg >> 32;
    fast_pool.pool[0] ^= reg ^ startup_high ^ irq;
    fast_pool.pool[1] ^= startup_time_ns ^ reg_high;
    fast_pool.pool[2] ^= rip;
    fast_pool.pool[3] ^= rip >> 32 ^ reg;


    uk_pr_crit("[0] = 0x%x\n", fast_pool.pool[0]);
    uk_pr_crit("[1] = 0x%x\n", fast_pool.pool[1]);
    uk_pr_crit("[2] = 0x%x\n", fast_pool.pool[2]);
    uk_pr_crit("[3] = 0x%x\n", fast_pool.pool[3]);

    fast_mix(&fast_pool);

    uk_pr_crit("[0] = 0x%x\n", fast_pool.pool[0]);
    uk_pr_crit("[1] = 0x%x\n", fast_pool.pool[1]);
    uk_pr_crit("[2] = 0x%x\n", fast_pool.pool[2]);
    uk_pr_crit("[3] = 0x%x\n", fast_pool.pool[3]);

    /*
     * 1) add prouced seed to the input pool -> call RDSEED
     * 2) add bytes from fast_pool in input_pool
     * 
     */

}