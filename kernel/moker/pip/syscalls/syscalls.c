#include <linux/syscalls.h>

#include "../../../sched/sched.h"
#include "../../trace/trace.h"
#include "syscalls.h"

SYSCALL_DEFINE0 (moker_pip_mutex_lock) {
  return sys_moker_pip_mutex_lock();
}

int sys_moker_pip_mutex_lock (void) {
#ifdef CONFIG_MOKER_MUTEX_PIP
  pip_mutex_lock();
#endif

  return 0;
}

SYSCALL_DEFINE0 (moker_pip_mutex_unlock) {
  return sys_moker_pip_mutex_unlock();
}

int sys_moker_pip_mutex_unlock (void) {
#ifdef CONFIG_MOKER_MUTEX_PIP
  pip_mutex_unlock();
#endif

  return 0;
}
