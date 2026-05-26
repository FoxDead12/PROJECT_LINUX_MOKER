#include <linux/syscalls.h>

#include "../../../sched/sched.h"
#include "../../trace/trace.h"

/**
 * System call used to set the period of task in sched_rm_entity
 */
SYSCALL_DEFINE1(moker_rm_set_period, unsigned long long, period) {
  printk("MOKER: moker_rm_set_period:[%lld][%d]\n", (long long) period, current->pid);
  return do_moker_rm_set_period(period);
}

int do_moker_rm_set_period (unsigned long long period) {
#ifdef CONFIG_MOKER_SCHED_RM_POLICY

  printk("MOKER: sys_moker_rm_set_period:[%lld][%d]\n", (long long) period, current->pid);

  // ... set was default ...
  current->rm.period = -1;

  // ... check policy of task if is SCHED_RM ...
  if ( rm_policy(current->policy) ) {
    current->rm.period = period;
  }

#endif
  return 0;
}
