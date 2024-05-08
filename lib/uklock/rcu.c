#include <uk/rcu.h>
#include <uk/plat/lcpu.h>


// Function to acquire read lock
void rcu_read_lock(void) {
        __lcpuidx idx=ukplat_lcpu_idx();
        rcu_flags[idx]=true;
}

// Function to release read lock
void rcu_read_unlock(void) {
    __lcpuidx idx=ukplat_lcpu_idx();
    rcu_flags[idx]=false;
    
}

// Function to synchronize RCU
void synchronize_rcu(void) {
    uk_pr_debug("\n in synchronize rcu\n");
    check_crit_flags(CONFIG_UKPLAT_LCPU_MAXCOUNT);
     uk_pr_debug("\n after synchronize rcu\n");
   }

void check_crit_flags(int lcpu_count) {

    bool result ;
    bool rcu_flags_temp[lcpu_count]; 
    COPY_ARRAY(rcu_flags,rcu_flags_temp,lcpu_count);
    LOGICAL_OR(rcu_flags_temp,lcpu_count,result);
    
    while (result == true) { 
     	   LOGICAL_OR(rcu_flags_temp,lcpu_count,result);
        } 
      return;
}
