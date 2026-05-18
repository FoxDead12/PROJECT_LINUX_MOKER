#ifndef __PCP_MUTEX_H_
#define __PCP_MUTEX_H_

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <asm/atomic.h>

struct pcp_mutex_wq {
  struct list_head    tasks;
  atomic_t            flag;
  raw_spinlock_t      lock;
  struct task_struct* owner;
  unsigned int        ceiling_prio;

  bool owner_prio_change;
  unsigned int owner_original_prio;
};

struct pcp_mutex_node {
  struct list_head node;
  struct task_struct* task;
};

struct pcp_mutex_wq* init_pcp_mutex(unsigned int ceiling_prio);
void lock_pcp_mutex(struct pcp_mutex_wq* wq);
int enqueue_pcp_mutex_task(struct pcp_mutex_wq* wq, struct task_struct* p);


#endif
