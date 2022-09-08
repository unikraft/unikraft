#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
#ifndef __has_feature
#define __has_feature(x) 0
#endif
/**
 * Swap macro that enforces a happens-before relationship with a corresponding
 * ATOMIC_LOAD.
 */
#if __has_feature(cxx_atomic)
#define ATOMIC_SWAP(addr, val)\
	__atomic_exchange_n(addr, val, __ATOMIC_ACQ_REL)
#elif __has_builtin(__sync_swap)
#define ATOMIC_SWAP(addr, val)\
	__sync_swap(addr, val)
#else
#define ATOMIC_SWAP(addr, val)\
	__sync_lock_test_and_set(addr, val)
#endif

#if __has_feature(cxx_atomic)
#define ATOMIC_LOAD(addr)\
	__atomic_load_n(addr, __ATOMIC_ACQUIRE)
#else
#define ATOMIC_LOAD(addr)\
	(__sync_synchronize(), *addr)
#endif
