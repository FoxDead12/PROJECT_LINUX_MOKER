#ifndef __PIP_MUTEX_H_
#define __PIP_MUTEX_H_

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <asm/atomic.h>

// ... struct to blocked processes
struct pip_mutex_wq {
  struct list_head tasks;
  atomic_t flag;            // 1 = mutex is locked, 0 = mutex is free
  raw_spinlock_t lock;
  struct task_struct* owner;
  bool owner_prio_change;
  unsigned int owner_original_prio;
  unsigned int owner_original_policy;
};

// ... struct to nodes of mutex ...
struct pip_mutex_node {
  struct list_head node;
  struct task_struct* task;
};

void init_pip_mutex(void);

// ... user space controll (maybe) ...
void lock_pip_mutex(void);
void unlock_pip_mutex(void);

int enqueue_pip_mutex_task(struct task_struct* p);
struct task_struct* dequeue_pip_mutex_task(void);

#endif
