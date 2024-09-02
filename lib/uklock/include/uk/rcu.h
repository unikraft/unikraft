#ifndef _UK_RCU_H_
#define _UK_RCU_H_
#include <uk/plat/lcpu.h>
#include <stdbool.h>

#include <uk/config.h>
#include <uk/assert.h>
#include <uk/errptr.h>

#include <uk/sched.h>
#ifdef __cplusplus
extern "C" {
#endif


struct rcu_head {
	struct rcu_head *next;
	void (*func)(struct rcu_head *head);
};

			
#define INIT_RCU_HEAD(ptr) do { \
    	INIT_LIST_HEAD(&(ptr)->list); (ptr)->func = NULL; \
		(ptr)->arg = NULL; \
		} while (0)

// Define memory barrier functions
#define smp_wmb() __asm__ __volatile__ ("" ::: "memory")
#define smp_read_barrier_depends() do {} while(0)

extern UKPLAT_PER_LCPU_DEFINE(bool, rcu_flags);

#define LOGICAL_OR(array, size, result) \
    do { \
        result = false; \
        for (int i = 0; i < size; ++i) { \
            result = result || array[i]; \
        } \
    } while (0)
    
#define COPY_ARRAY(src, dest, size) \
    do { \
        for (int i = 0; i < size; ++i) { \
            dest[i] = src[i]; \
        } \
    } while (0)
    
void check_crit_flags(int lcpu_count); 


// Define RCU API functions
void rcu_read_lock(void);
void rcu_read_unlock(void);
void synchronize_rcu(void);

#define rcu_assign_pointer(p, v)	({ \
							smp_wmb(); \
							(p) = (v); \
						})

#define rcu_dereference(p)     ({ \
					typeof(p) _____p1 = p; \
					smp_read_barrier_depends(); \
					(_____p1); \
					})

#ifdef __cplusplus
}
#endif


#endif /* _UK_RCU_H_ */
