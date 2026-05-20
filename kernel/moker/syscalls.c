#include <linux/syscalls.h>
#include "trace.h"
#include "pip/pip_mutex.h"
#include "pcp/pcp_mutex.h"

SYSCALL_DEFINE1 (moker_tracing, unsigned int, enable) {
  printk("MOKER: moker_tracing:[%d][%d]\n", (int) enable, current->pid);
  return do_moker_tracing(enable);
}

SYSCALL_DEFINE0 (moker_mutex_lock) {
  printk("MOKER: moker_tracing:[%d] lock pip mutex\n", current->pid);
  return sys_moker_mutex_lock();
}

SYSCALL_DEFINE0 (moker_mutex_unlock) {
  printk("MOKER: moker_tracing:[%d] unlock pip mutex\n", current->pid);
  return sys_moker_mutex_unlock();
}


int do_moker_tracing (unsigned int enable) {
#ifdef CONFIG_MOKER_TRACING
  printk("MOKER: sys_moker_tracing:[%d][%d]\n", (int) enable, current->pid);
  enable_tracing(enable);
#endif

  return 0;
}

int sys_moker_mutex_lock () {
#ifdef CONFIG_MOKER_MUTEX_PIP
  lock_pip_mutex();
#endif

  return 0;
}

int sys_moker_mutex_unlock (){
#ifdef CONFIG_MOKER_MUTEX_PIP
  unlock_pip_mutex();
#endif

  return 0;
}






SYSCALL_DEFINE0(moker_pcp_mutex_lock) {
  return sys_moker_pcp_mutex_lock();
}

int sys_moker_pcp_mutex_lock () {
  printk("MOKER: sys_moker_pcp_mutex_lock:[%d]\n", current->pid);

#ifdef CONFIG_MOKER_MUTEX_PCP
  lock_pcp_mutex();
#endif
  return 0;
}


SYSCALL_DEFINE0(moker_pcp_mutex_unlock) {
  return sys_moker_pcp_mutex_unlock();
}

int sys_moker_pcp_mutex_unlock () {
  printk("MOKER: sys_moker_pcp_mutex_unlock:[%d]\n", current->pid);
#ifdef CONFIG_MOKER_MUTEX_PCP
  unlock_pcp_mutex();
#endif
  return 0;
}
