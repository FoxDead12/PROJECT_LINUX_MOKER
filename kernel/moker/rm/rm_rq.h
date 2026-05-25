#ifndef __RM_RQ_H_
#define __RM_RQ_H_

// #include <linux/sched.h>
// #include <linux/list.h>
// #include <linux/spinlock.h>
//
// struct lf_rq{
//   struct list_head tasks;
//   struct task_struct *task;
//   raw_spinlock_t lock;
//   unsigned nr_running;
// };
//

struct rm_rq {
};

void init_rm_rq (struct rm_rq* rq);

#endif
