#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "pcp_mutex.h"
#include "../trace.h"

struct pcp_mutex_wq*
init_pcp_mutex (unsigned int ceiling_prio) {

  struct pcp_mutex_wq* wq = kmalloc(sizeof(struct pcp_mutex_wq), GFP_KERNEL);

  INIT_LIST_HEAD(&wq->tasks);
  raw_spin_lock_init(&wq->lock);
  atomic_set(&wq->flag, 0); // ... start variable atomic 0
  wq->owner = NULL;

  // ... TODO: validate prio number, is necessary safe range check?
  wq->ceiling_prio = ceiling_prio;

  return wq;
}

/**
 * Method used to a process lock a resource
 */
void
lock_pcp_mutex (struct pcp_mutex_wq* wq) {

  struct task_struct* p = current;

  if ( atomic_add_unless(&wq->flag, 1, 1) ) {
    // ... current lock mutext (win)
    printk("MOKER: lock_pcp_mutex[%d] first try catch mutex\n", p->pid);
  } else {
    // ... current stay bloqued (lose)

    enqueue_pcp_mutex_task(wq, p);
    printk("MOKER: lock_pip_mutex[%d] enqueue task\n", p->pid);

    while ( !atomic_add_unless(&wq->flag, 1, 1) ) {
      set_current_state(TASK_INTERRUPTIBLE);
      printk("MOKER: lock_pcp_mutex[%d] task in while will wait to set flag\n", p->pid);
      schedule();
    }

    printk("MOKER: lock_pcp_mutex[%d] set flag after while\n", p->pid);
    set_current_state(TASK_RUNNING);
  }

  if ( wq->owner != NULL && p->rt_priority < wq->ceiling_prio ) {
    // ... update prio to ceiling prio
    if ( !wq->owner_prio_change ) {
      wq->owner_prio_change = true;
      wq->owner_original_prio = p->rt_priority;
    }

    printk("MOKER: lock_pcp_mutex[%d] update prio of mutex owner new: %d old: %d\n", p->pid, p->rt_priority, wq->owner_original_prio);

    struct sched_param param;
    param.sched_priority = wq->ceiling_prio;
    sched_setscheduler(p, p->policy, &param);
    printk("MOKER: lock_pcp_mutex[%d] set new params to scheduler\n", p->pid);
  }

  wq->owner = p;

#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_LOCK, p, 5);
#endif

}

void
unlock_pcp_mutex (struct pcp_mutex_wq* wq) {
  struct task_struct* p = current;
  struct task_struct* t = NULL;

  t = dequeue_pcp_mutex_task(wq);
  printk("MOKER: unlock_pcp_mutex[%d] deuque task\n", p->pid);

  atomic_set(&wq.flag, 0);
  printk("MOKER: unlock_pcp_mutex[%d] set flag to 0 \n", p->pid);

  // ... restore prio of task
  if ( wq.owner_prio_change ) {
    printk("MOKER: unlock_pip_mutex[%d] restore main setting prio and policy default: %d temporary: %d\n", p->pid, wq.owner_original_prio, p->rt_priority);
    struct sched_param param;
    param.sched_priority = wq->owner_original_prio;
    sched_setscheduler(p, p->policy, &param);
  }

  printk("MOKER: unlock_pip_mutex[%d] restore mutex settings\n", p->pid);
  wq->owner = NULL;
  wq->owner_original_prio = 0;

  if ( t ) {
    if ( !wake_up_process(t) ) {
      printk(KERN_ERR "BUG: wake up process failed: %d\n", p->pid);
    }
  }

#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_UNLOCK, p, 7);
#endif

}

/**
 * Method to add a new process/task to waiting queue
 */
int
enqueue_pcp_mutex_task (struct pcp_mutex_wq* wq, struct task_struct* p) {

  int ret = -1;
  bool inserted = false;
  struct pcp_mutex_node* pos = NULL;
  struct pcp_mutex_node* t = kmalloc(sizeof(struct pcp_mutex_node), GFP_KERNEL);

    if ( t ) {
    t->task = p;
    raw_spin_lock(&wq->lock);

    list_for_each_entry(pos, &wq->tasks, node) {
      if ( p->prio < pos->task->prio ) {
        list_add_tail(&t->node, &pos->node);
        inserted = true;
        break;
      }
    }

    if ( !inserted ) {
      list_add_tail(&t->node, &wq->tasks);
    }

    raw_spin_unlock(&wq->lock);
    ret = 0;
  }

#ifdef CONFIG_MOKER_TRACING
  moker_trace(ENQUEUE_WQ, p, 4);
#endif

  return ret;

}

struct task_struct*
dequeue_pcp_mutex_task (struct pcp_mutex_wq* wq) {

  printk("MOKER: dequeue_pcp_mutex_task[%d] start\n", current->pid);

  struct task_struct* p = NULL;
  struct pip_mutex_node* t = NULL;

  raw_spin_lock(&wq->lock);

  // ... this logic is from LIFO need change ...
  if ( !list_empty(&wq->tasks) ) {
    // ... get first element of list ...
    t = list_first_entry(&wq->tasks, struct pip_mutex_node, node);

    // ... get task ...
    p = t->task;
    printk("MOKER: dequeue_pcp_mutex_task[%d] dequeue tast %d\n", current->pid, p->pid);

    list_del(&t->node);
  } else {
    printk("MOKER: dequeue_pcp_mutex_task[%d] queue is empty\n", current->pid);
  }

  raw_spin_unlock(&wq->lock);

  printk("MOKER: dequeue_pcp_mutex_task[%d] out spin lock\n", current->pid);

  if (t) {
    kfree(t);
  }

#ifdef CONFIG_MOKER_TRACING
  if ( p ) {
    moker_trace(DEQUEUE_WQ, p, 6);
  }
#endif

  return p;

}
