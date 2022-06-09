#include <uk/plat/spinlock.h>
#include <uk/entropy.h>
#include <uk/print.h>
#include <uk/plat/time.h>
#include <uk/bitops.h>
#include <uk/init.h>
#include <uk/cpuid.h>
#include <uk/rdrand.h>
#include <uk/blake3.h>
#include <string.h>

#define EXTRACT_SIZE    	10	
#define INPUT_POOL_SHIFT	12
#define INPUT_POOL_WORDS	(1 << (INPUT_POOL_SHIFT-5))
#define INPUT_POOL_OUTPUT_HASH_SIZE 8
#define OUTPUT_POOL_SHIFT	10
#define OUTPUT_POOL_WORDS	(1 << (OUTPUT_POOL_SHIFT-5))
#define INIT_ENTROPY_SIZE 	32
#define ENTROPY_SHIFT 3
static struct poolinfo {
	__u32 poolbitshift, poolwords, poolbytes, poolbits, poolfracbits;
	__u32 tap1, tap2, tap3, tap4, tap5;
} poolinfo = {
	12,	128, 512, 4096, 32758, 104,	76,	51,	25,	1
};

struct entropy_store {
	/* read-only data: */
	const struct poolinfo *poolinfo;
	__u32 *pool;
	const char *name;

    __spinlock spinlock;
	__u16 add_ptr;
	__u16 input_rotate;
	__u32 entropy_count;
	__u32 entropy_total;
	__u32 initialized:1;
	__u32 last_data_init:1;
	__u8 last_data[EXTRACT_SIZE];
};

struct fast_pool {
    __u32 pool[4];
    __u32 last;
    __u16 reg_index;
    __u8 count;
};


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

/*
 * the minimum ammount of entropy for which reading is allowed
*/
static int random_read_minimum_bits = 64;
static __u32 input_pool_data[INPUT_POOL_WORDS];
static __u32 const twist_table[8] = {
	0x00000000, 0x3b6e20c8, 0x76dc4190, 0x4db26158,
	0xedb88320, 0xd6d6a3e8, 0x9b64c2b0, 0xa00ae278 };

static struct fast_pool fast_pool = {
    .reg_index = 0,
};

static struct entropy_store input_pool = {
	.poolinfo = &poolinfo,
	.name = "input",
	.pool = input_pool_data
};


static __u8 get_hw_entropy(__u32* data, __u32 len) {
	__u8 (*func)(__u32*);
	__u8 entropy_count = 0, ret;

	if (is_RDSEED_available()) {
		func = &uk_hwrand_rdseed;
	} else if (is_RDRAND_available()) {
		func = &uk_hwrand_rdrand;
	} else {
		return entropy_count;
	}

	for (ssize_t i = 0; i < len; i++) {
		ret = func(&data[i]);
		entropy_count += ret;
	}

	return entropy_count;
}


static void _mix_pool_bytes(struct entropy_store *r, const void *in,
			    int nbytes)
{
	__u32 i, tap1, tap2, tap3, tap4, tap5;
	__u32 input_rotate;
	__u32 wordmask = r->poolinfo->poolwords - 1;
	const char *bytes = (const char*)in;
	__u32 w;

	tap1 = r->poolinfo->tap1;
	tap2 = r->poolinfo->tap2;
	tap3 = r->poolinfo->tap3;
	tap4 = r->poolinfo->tap4;
	tap5 = r->poolinfo->tap5;

	input_rotate = r->input_rotate;
	i = r->add_ptr;

	/* mix one byte at a time to simplify size handling and churn faster */
	while (nbytes--) {
		w = uk_rol32(*bytes++, input_rotate);
		i = (i - 1) & wordmask;

		/* XOR in the various taps */
		w ^= r->pool[i];
		w ^= r->pool[(i + tap1) & wordmask];
		w ^= r->pool[(i + tap2) & wordmask];
		w ^= r->pool[(i + tap3) & wordmask];
		w ^= r->pool[(i + tap4) & wordmask];
		w ^= r->pool[(i + tap5) & wordmask];

		/* Mix the result back in with a twist */
		r->pool[i] = (w >> 3) ^ twist_table[w & 7];

		/*
		 * Normally, we add 7 bits of rotation to the pool.
		 * At the beginning of the pool, add an extra 7 bits
		 * rotation, so that successive passes spread the
		 * input bits across the pool evenly.
		 */
		input_rotate = (input_rotate + (i ? 7 : 14)) & 31;
	}

	r->input_rotate = input_rotate;
	r->add_ptr = i;
}

/*
 * This function decides how many bytes to actually take from the
 * given pool, and also debits the entropy count accordingly.
 */
static size_t __uk_account(struct entropy_store *r, size_t nbytes)
{
	int entropy_count, flags;
	size_t extracted_frac, nfrac, min_frac;
	ssize_t max_frac;

	ukplat_spin_lock_irqsave(&r->spinlock, flags);
	entropy_count = r->entropy_count;
	ukplat_spin_unlock_irqrestore(&r->spinlock, flags);


	nfrac = nbytes << (ENTROPY_SHIFT + 3);
	min_frac = random_read_minimum_bits << 3;
	max_frac = entropy_count - min_frac;

	/* never pull more than available */
	if (max_frac < 0) {
		return 0;
	}

	extracted_frac = nfrac > (size_t)max_frac ? (size_t)max_frac : nfrac;

	entropy_count -= extracted_frac;
	if (unlikely(entropy_count < 0)) {
		uk_pr_crit("random: negative entropy count: pool %s count %d\n",
			r->name, entropy_count);
		entropy_count = 0;
	}

	ukplat_spin_lock_irqsave(&r->spinlock, flags);
	r->entropy_count = entropy_count;
	ukplat_spin_unlock_irqrestore(&r->spinlock, flags);

	return (extracted_frac >> (ENTROPY_SHIFT + 3));
}

/*
 * Credit (or debit) the entropy store with n bits of entropy.
 */
static void _uk_credit_entropy_bits(struct entropy_store *r, __u8 nbits)
{
	const int pool_size = r->poolinfo->poolfracbits;
	/* The +2 corresponds to the /4 in the denominator */
	const int s = r->poolinfo->poolbitshift + ENTROPY_SHIFT + 2;
	int nfrac = nbits << ENTROPY_SHIFT;
	int pnfrac, anfrac, add, flags;
	int entropy_count;


	if (nbits == 0)
		return;
	
	ukplat_spin_lock_irqsave(&r->spinlock, flags);
	entropy_count = r->entropy_count;
	ukplat_spin_unlock_irqrestore(&r->spinlock, flags);

	if (nfrac < 0) {
		/* Debit */
		entropy_count += nfrac;
	} else {
		/*
		 * Credit: we have to account for the possibility of
		 * overwriting already present entropy.	 Even in the
		 * ideal case of pure Shannon entropy, new contributions
		 * approach the full value asymptotically:
		 *
		 * entropy <- entropy + (pool_size - entropy) *
		 *	(1 - exp(-add_entropy/pool_size))
		 *
		 * For add_entropy <= pool_size/2 then
		 * (1 - exp(-add_entropy/pool_size)) >=
		 *    (add_entropy/pool_size)*0.7869...
		 * so we can approximate the exponential with
		 * 3/4*add_entropy/pool_size and still be on the
		 * safe side by adding at most pool_size/2 at a time.
		 *
		 * The use of pool_size-2 in the while statement is to
		 * prevent rounding artifacts from making the loop
		 * arbitrarily long; this limits the loop to log2(pool_size)*2
		 * turns no matter how large nbits is.
		 */
		pnfrac = nfrac;

		do {
			anfrac = pnfrac > pool_size / 2 ? pool_size / 2 : pnfrac;
			add = ((pool_size - entropy_count) * anfrac * 3) >> s;

			entropy_count += add;
			pnfrac -= anfrac;
		} while (unlikely(entropy_count < pool_size - 2 && pnfrac));
	}

	if (unlikely(entropy_count < 0)) {
		uk_pr_crit("random: negative entropy/overflow: pool %s count %d\n",
			r->name, entropy_count);
		entropy_count = 0;
	} else if (entropy_count > pool_size)
		entropy_count = pool_size;

	ukplat_spin_lock_irqsave(&r->spinlock, flags);
	r->entropy_count = entropy_count;
	ukplat_spin_unlock_irqrestore(&r->spinlock, flags);

}
/*
 * This function does the actual extraction for extract_entropy and
 * extract_entropy_user.
 *
 * Note: we assume that .poolwords is a multiple of 16 words.
 */
static void __uk_extract_buf(struct entropy_store *r, __u8 *out)
{
	ssize_t i;
	blake3_hasher hasher;
	__u32 hash_input[INPUT_POOL_WORDS];
	__u32 hash_output[INPUT_POOL_OUTPUT_HASH_SIZE];
	__u32 flags;

    blake3_hasher_init(&hasher);

	/*
	 * If we have an architectural hardware random number
	 * generator, use it for BLAKE's initial vector
	 */
	get_hw_entropy(hash_input, INPUT_POOL_WORDS);

	/* build hash input across the pool, 1 word at a time */
	ukplat_spin_lock_irqsave(&r->spinlock, flags);
	for (i = 0; i < r->poolinfo->poolwords; i++)
		hash_input[i] ^= r->pool[i];

	/* Update the state of the hasher */
	blake3_hasher_update(&hasher, (__uint8_t*)hash_input, sizeof(__u32) * INPUT_POOL_WORDS);


	/* Get the hash of the input pool */
	blake3_hasher_finalize(&hasher, (__uint8_t*)hash_output, sizeof(__u32) * INPUT_POOL_OUTPUT_HASH_SIZE);

	/*
	 * We mix the hash back into the pool to prevent backtracking
	 * attacks (where the attacker knows the state of the pool
	 * plus the current outputs, and attempts to find previous
	 * ouputs), unless the hash function can be inverted. By
	 * mixing at least a SHA1 worth of hash data back, we make
	 * brute-forcing the feedback as hard as brute-forcing the
	 * hash.
	 */
	_mix_pool_bytes(r, hash_output, sizeof(__u32) * INPUT_POOL_OUTPUT_HASH_SIZE);
	ukplat_spin_unlock_irqrestore(&r->spinlock, flags);

	memset(hash_input, 0, sizeof(__u32) * INPUT_POOL_WORDS);

	/*
	 * In case the hash function has some recognizable output
	 * pattern, we fold it in half. Thus, we always feed back
	 * twice as much data as we output.
	 */
	hash_output[0] ^= hash_output[4];
	hash_output[1] ^= hash_output[5];
	hash_output[2] ^= hash_output[6];
	hash_output[3] ^= hash_output[7];

	memcpy(out, hash_output, EXTRACT_SIZE);
	memset(hash_output, 0, sizeof(__u32) * INPUT_POOL_OUTPUT_HASH_SIZE);
}


static size_t _extract_entropy(struct entropy_store *r, void *buf,
				size_t nbytes)
{
	size_t ret = 0, size;
	__u8 tmp[EXTRACT_SIZE];
	__u32 flags;

	nbytes = __uk_account(r, nbytes);
	while (nbytes) {
		__uk_extract_buf(r, tmp);

		ukplat_spin_lock_irqsave(&r->spinlock, flags);
		if (r->last_data_init &&
			!memcmp(tmp, r->last_data, EXTRACT_SIZE)) {
			uk_pr_crit("Hardware RNG duplicated output!\n");
			return 0;
		}
		memcpy(r->last_data, tmp, EXTRACT_SIZE);
		r->last_data_init = 1;
		ukplat_spin_unlock_irqrestore(&r->spinlock, flags);
	

		size = nbytes > EXTRACT_SIZE ? EXTRACT_SIZE : nbytes;
		memcpy(buf, tmp, size);
		nbytes -= size;
		buf += size;
		ret += size;
	}

	/* Wipe data just returned from memory */
	memset(tmp, 0, sizeof(tmp));

	return ret;
}

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
	__u32 hw_entropy, ret = 0;
	__u8 credit = 0;


	if (!input_pool.initialized) {
		return;
	}

    get_registers(&regs);
    reg = get_reg(&fast_pool, &regs);

    reg_high = reg >> 32;
    startup_high = reg >> 32;
    fast_pool.pool[0] ^= reg ^ startup_high ^ irq;
    fast_pool.pool[1] ^= startup_time_ns ^ reg_high;
    fast_pool.pool[2] ^= rip;
    fast_pool.pool[3] ^= rip >> 32 ^ reg;

    fast_mix(&fast_pool);
	fast_pool.count++;
	
	/*
	 * Wait for 10 interrupts before adding the entropy in input_pool
	 */
	if (fast_pool.count < 10) {
		return;
	}

	/*
	 * If we have architectural seed generator, produce a seed and
	 * add it to the pool.
	 */
	if (is_RDSEED_available()) {
		ret = uk_hwrand_rdseed(&hw_entropy);
	}

	if (ret == 0) {
		if (is_RDRAND_available()) {
			ret = uk_hwrand_rdrand(&hw_entropy);
		}
	}
	
	if (ret){
		credit += sizeof(__u32);
		_mix_pool_bytes(&input_pool, &hw_entropy, sizeof(__u32));
	}

	/*
	 * Add fast pool entropy to the input pool
	 */
	_mix_pool_bytes(&input_pool, fast_pool.pool, sizeof(fast_pool.pool));
	credit += sizeof(fast_pool.pool);

	fast_pool.count = 0;

	_uk_credit_entropy_bits(&input_pool, credit);
}

size_t uk_entropy_generate_bytes(void *buf, size_t buflen)
{
	return _extract_entropy(&input_pool, buf, buflen);
}


int uk_entropy_init(void) {
	__u32 initial_entropy[INIT_ENTROPY_SIZE];
	__u8 entropy_count;

	input_pool.initialized = 1;
	entropy_count = get_hw_entropy(initial_entropy, INIT_ENTROPY_SIZE);
	ukarch_spin_init(&input_pool.spinlock);

	_mix_pool_bytes(&input_pool, &initial_entropy, entropy_count);
	_uk_credit_entropy_bits(&input_pool, entropy_count * sizeof(__u32));

	return 0;
}
