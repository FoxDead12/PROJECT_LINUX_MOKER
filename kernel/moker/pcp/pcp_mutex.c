#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "pcp_mutex.h"
#include "../trace.h"

struct pcp_mutex_wq wq_pcp;

void
init_pcp_mutex (void) {

  INIT_LIST_HEAD(&wq_pcp.tasks);
  raw_spin_lock_init(&wq_pcp.lock);
  atomic_set(&wq_pcp.flag, 0); // ... start variable atomic 0
  wq_pcp.owner = NULL;

  // ... TODO: validate prio number, is necessary safe range check?
  wq_pcp.ceiling_prio = 90;

  printk("MOKER: init_pcp_mutex[%d] just init\n", current->pid);
}

/**
 * Method used to a process lock a resource
 */
int
lock_pcp_mutex (void) {

  struct task_struct* p = current;

  if ( atomic_add_unless(&wq_pcp.flag, 1, 1) ) {
    // ... current lock mutext (win)
    printk("MOKER: lock_pcp_mutex[%d] first try catch mutex\n", p->pid);
  } else {
    // ... current stay bloqued (lose)

    enqueue_pcp_mutex_task(p);
    printk("MOKER: lock_pip_mutex[%d] enqueue task\n", p->pid);

    while ( !atomic_add_unless(&wq_pcp.flag, 1, 1) ) {
      set_current_state(TASK_INTERRUPTIBLE);
      printk("MOKER: lock_pcp_mutex[%d] task in while will wait to set flag\n", p->pid);
      schedule();
    }

    printk("MOKER: lock_pcp_mutex[%d] set flag after while\n", p->pid);
    set_current_state(TASK_RUNNING);
  }

  if ( wq_pcp.owner != NULL && p->rt_priority < wq_pcp.ceiling_prio ) {
    // ... update prio to ceiling prio
    if ( !wq_pcp.owner_prio_change ) {
      wq_pcp.owner_prio_change = true;
      wq_pcp.owner_original_prio = p->rt_priority;
    }

    printk("MOKER: lock_pcp_mutex[%d] update prio of mutex owner new: %d old: %d\n", p->pid, p->rt_priority, wq_pcp.owner_original_prio);

    struct sched_param param;
    param.sched_priority = wq_pcp.ceiling_prio;
    sched_setscheduler(p, p->policy, &param);
    printk("MOKER: lock_pcp_mutex[%d] set new params to scheduler\n", p->pid);
  }

  wq_pcp.owner = p;

#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_LOCK, p, 5);
#endif

  return 0;
}

int
unlock_pcp_mutex (void) {
  struct task_struct* p = current;
  struct task_struct* t = NULL;

  t = dequeue_pcp_mutex_task();
  printk("MOKER: unlock_pcp_mutex[%d] deuque task\n", p->pid);

  atomic_set(&wq_pcp.flag, 0);
  printk("MOKER: unlock_pcp_mutex[%d] set flag to 0 \n", p->pid);

  // ... restore prio of task
  if ( wq_pcp.owner_prio_change ) {
    printk("MOKER: unlock_pip_mutex[%d] restore main setting prio and policy default: %d temporary: %d\n", p->pid, wq_pcp.owner_original_prio, p->rt_priority);
    struct sched_param param;
    param.sched_priority = wq_pcp.owner_original_prio;
    sched_setscheduler(p, p->policy, &param);
  }

  printk("MOKER: unlock_pip_mutex[%d] restore mutex settings\n", p->pid);
  wq_pcp.owner = NULL;
  wq_pcp.owner_original_prio = 0;

  if ( t ) {
    if ( !wake_up_process(t) ) {
      printk(KERN_ERR "BUG: wake up process failed: %d\n", p->pid);
    }
  }

#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_UNLOCK, p, 7);
#endif

  return 0;
}

/**
 * Method to add a new process/task to waiting queue
 */
int
enqueue_pcp_mutex_task (struct task_struct* p) {

  int ret = -1;
  bool inserted = false;
  struct pcp_mutex_node* pos = NULL;
  struct pcp_mutex_node* t = kmalloc(sizeof(struct pcp_mutex_node), GFP_KERNEL);

    if ( t ) {
    t->task = p;
    raw_spin_lock(&wq_pcp.lock);

    list_for_each_entry(pos, &wq_pcp.tasks, node) {
      if ( p->prio < pos->task->prio ) {
        list_add_tail(&t->node, &pos->node);
        inserted = true;
        break;
      }
    }

    if ( !inserted ) {
      list_add_tail(&t->node, &wq_pcp.tasks);
    }

    raw_spin_unlock(&wq_pcp.lock);
    ret = 0;
  }

#ifdef CONFIG_MOKER_TRACING
  moker_trace(ENQUEUE_WQ, p, 4);
#endif

  return ret;

}

struct task_struct*
dequeue_pcp_mutex_task (void) {

  printk("MOKER: dequeue_pcp_mutex_task[%d] start\n", current->pid);

  struct task_struct* p = NULL;
  struct pcp_mutex_node* t = NULL;

  raw_spin_lock(&wq_pcp.lock);

  // ... this logic is from LIFO need change ...
  if ( !list_empty(&wq_pcp.tasks) ) {
    // ... get first element of list ...
    t = list_first_entry(&wq_pcp.tasks, struct pcp_mutex_node, node);

    // ... get task ...
    p = t->task;
    printk("MOKER: dequeue_pcp_mutex_task[%d] dequeue tast %d\n", current->pid, p->pid);

    list_del(&t->node);
  } else {
    printk("MOKER: dequeue_pcp_mutex_task[%d] queue is empty\n", current->pid);
  }

  raw_spin_unlock(&wq_pcp.lock);

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
