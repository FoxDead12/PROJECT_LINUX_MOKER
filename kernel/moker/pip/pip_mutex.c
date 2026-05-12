#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "pip_mutex.h"
#include "../trace.h"

// ... wait queue ...
struct pip_mutex_wq wq;

void
init_pip_mutex (void) {

  INIT_LIST_HEAD(&wq.tasks); // ... init list

  raw_spin_lock_init(&wq.lock); // ... init spin lock

  atomic_set(&wq.flag, 0); // ... start variable atomic 0

  wq.owner = NULL;  // ... init owner
  wq.owner_prio_change = false;
  wq.owner_original_prio = 0;
  wq.owner_original_policy = 0;
}

/**
 * Method used to a process lock a resource
 */
void
lock_pip_mutex (void) {

  struct task_struct* p = current;

  /**
  * Implemente PIP logic
  * 1º Run bigest priority task
  * 2º If task running has bigges priority need up
  */
  if ( atomic_add_unless(&wq.flag, 1, 1) ) {
    // ... task will keep the shared resource ...
    printk("MOKER: lock_pip_mutex[%d] first try catch mutex\n", p->pid);
  } else {
    // ... task can't run, need check if is necessary up the prio of owner ...
    if ( wq.owner != NULL && p->prio < wq.owner->prio ) {
      // ... need up the prio of owner task
      if ( !wq.owner_prio_change ) {
        wq.owner_prio_change = true;
        wq.owner_original_prio = wq.owner->rt_priority;
        wq.owner_original_policy = wq.owner->policy;
        printk("MOKER: lock_pip_mutex[%d] update prio and policy of mutex owner\n", p->pid);
      }

      struct sched_param param;
      param.sched_priority = p->rt_priority;
      sched_setscheduler(wq.owner, p->policy, &param);
      printk("MOKER: lock_pip_mutex[%d] set new params to scheduler\n", p->pid);
    }

    enqueue_pip_mutex_task(p);
    printk("MOKER: lock_pip_mutex[%d] enqueue task\n", p->pid);

    // ... wait until lock resource ...
    while ( !atomic_add_unless(&wq.flag, 1, 1) ) {
      set_current_state(TASK_INTERRUPTIBLE);
      printk("MOKER: lock_pip_mutex[%d] task in while will wait to set flag\n", p->pid);
      schedule();
    }

    printk("MOKER: lock_pip_mutex[%d] set flag after while\n", p->pid);
  }

#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_LOCK, p, -1);
#endif

  set_current_state(TASK_RUNNING);
  wq.owner = p;

}

void
unlock_pip_mutex (void) {
  struct task_struct* p = current;
  struct task_struct* t = NULL;

  t = dequeue_pip_mutex_task();
  printk("MOKER: unlock_pip_mutex[%d] deuque task\n", p->pid);

  atomic_set(&wq.flag, 0);
  printk("MOKER: unlock_pip_mutex[%d] set flag to 0 \n", p->pid);

  if ( t ) {
    if ( !wake_up_process(t) ) {
      printk(KERN_ERR "BUG: wake up process failed: %d\n", p->pid);
    }
  }

  // ... restore prio and policy of thread
  if ( wq.owner_prio_change ) {
    printk("MOKER: unlock_pip_mutex[%d] restore main setting prio and policy\n", p->pid);
    struct sched_param param;
    param.sched_priority = wq.owner_original_prio;
    sched_setscheduler(p, wq.owner_original_policy, &param);
  }

  printk("MOKER: unlock_pip_mutex[%d] restore mutex settings\n", p->pid);
  wq.owner = NULL;
  wq.owner_prio_change = false;
  wq.owner_original_prio = 0;
  wq.owner_original_policy = 0;

// #ifdef CONFIG_MOKER_TRACING
//   moker_trace(MUTEX_UNLOCK, p, -1);
// #endif

}

/**
 * Method to add a new process/task to waiting queue
 */
int
enqueue_pip_mutex_task (struct task_struct* p) {

  int ret = -1;
  bool inserted = false;
  struct pip_mutex_node* pos = NULL;
  struct pip_mutex_node* t = kmalloc(sizeof(struct pip_mutex_node), GFP_KERNEL);

  if ( t ) {
    t->task = p;
    raw_spin_lock(&wq.lock);

    list_for_each_entry(pos, &wq.tasks, node) {
      if ( p->prio < pos->task->prio ) {
        list_add_tail(&t->node, &pos->node);
        inserted = true;
        break;
      }
    }

    if ( !inserted ) {
      list_add_tail(&t->node, &wq.tasks);
    }

    raw_spin_unlock(&wq.lock);
    ret = 0;
  }

#ifdef CONFIG_MOKER_TRACING
  moker_trace(ENQUEUE_WQ, p, -1);
#endif

  return ret;

}


/**
 * Method to remove a process/task from waiting queue
 */
struct task_struct*
dequeue_pip_mutex_task (void) {

  struct task_struct* p = NULL;
  struct pip_mutex_node* t = NULL;

  raw_spin_lock(&wq.lock);

  // ... this logic is from LIFO need change ...
  if ( !list_empty(&wq.tasks) ) {
    // ... get first element of list ...
    t = list_first_entry(&wq.tasks, struct pip_mutex_node, node);
    // ... get task ...
    p = t->task;
  }

  raw_spin_unlock(&wq.lock);

  if (t) {
    list_del(&t->node);
    kfree(t);
  }

// #ifdef CONFIG_MOKER_TRACING
//   if ( p ) {
//     moker_trace(DEQUEUE_WQ, p, -1);
//   }
// #endif

  return p;
}
