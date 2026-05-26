#ifndef __RM_RQ_H_
#define __RM_RQ_H_

#include <linux/sched.h>
// #include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/spinlock.h>

struct rm_rq {
  struct list_head tasks;
  struct task_struct *task;
  raw_spinlock_t lock;
  unsigned nr_running;

//   struct rb_node root;
//   raw_spinlock_t lock;
//   unsigned nr_running;
};

void init_rm_rq (struct rm_rq* rq);

#endif
