#include <linux/syscalls.h>
#include "../trace.h"

SYSCALL_DEFINE1(moker_tracing, unsigned int, enable) {
  printk("MOKER: moker_tracing:[%d][%d]\n", (int) enable, current->pid);
  return do_moker_tracing(enable);
}

int do_moker_tracing (unsigned int enable){
  #ifdef CONFIG_MOKER_TRACING
  printk("MOKER: sys_moker_tracing:[%d][%d]\n", (int) enable, current->pid);
  enable_tracing(enable);
  #endif

  return 0;
}
