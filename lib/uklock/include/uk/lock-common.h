#include <uk/plat/lcpu.h>

#define _IRQFNAME(funcname) funcname##_irqf

#define _LOCK_IRQF(locktype, funcname)				\
	static inline									\
	unsigned int _IRQFNAME(funcname)(locktype lock)	\
	{												\
		unsigned int irqf = ukplat_lcpu_save_irqf();\
		funcname(lock);								\
		return irqf;								\
	}

#define _TRYLOCK_IRQF(locktype, funcname)					\
	static inline											\
	unsigned int _IRQFNAME(funcname)(locktype lock, int *r)	\
	{														\
		unsigned int irqf = ukplat_lcpu_save_irqf();		\
		*r = funcname(lock);								\
		if (*r == 0)										\
			ukplat_lcpu_restore_irqf(irqf);					\
		return irqf;										\
	}

#define _UNLOCK_IRQF(locktype, funcname)						\
	static inline												\
	void _IRQFNAME(funcname)(locktype lock, unsigned int irqf)	\
	{															\
		funcname(lock);											\
		ukplat_lcpu_restore_irqf(irqf);							\
	}
