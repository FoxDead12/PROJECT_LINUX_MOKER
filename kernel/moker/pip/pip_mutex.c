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
 * Method used to a task lock the resource
 */
void
lock_pip_mutex (void) {

  struct task_struct* p = current;
  bool scheduler_up = false;

  /**
  * Implemente PIP logic
  * 1º Run bigest priority task
  * 2º If task running has bigges priority need up
  */

  if ( atomic_add_unless(&wq.flag, 1, 1) ) {
    printk("MOKER[%d][PIP] enqueue task\n", p->pid);
    wq.owner = p;
    return;
  }

  // ... update wait queue params
  raw_spin_lock(&wq.lock);

  if ( wq.owner ) {
    if ( p->prio < wq.owner->prio ) {
      if ( !wq.owner_prio_change ) {
        wq.owner_prio_change = true;
        wq.owner_original_prio = wq.owner->rt_priority;
        wq.owner_original_policy = wq.owner->policy;
        printk("MOKER[%d][PIP] store owner original props\n", p->pid);
      }
      scheduler_up = true;
    }
  }

  raw_spin_unlock(&wq.lock);

  // ... update scheduler ...
  if ( scheduler_up ) {
    struct sched_param param;
    param.sched_priority = p->rt_priority;
    sched_setscheduler(wq.owner, p->policy, &param);
    printk("MOKER[%d][PIP] update scheduler\n", p->pid);
  }

  enqueue_pip_mutex_task(p);
  printk("MOKER[%d][PIP] enqueue\n", p->pid);

  for (;;) {
    // ... force task to set status sleep
    set_current_state(TASK_INTERRUPTIBLE);

    // ... check if win mutex/resource
    if ( atomic_add_unless(&wq.flag, 1, 1) ) {
      break;
    }

    printk("MOKER[%d][PIP] task will sleep\n", p->pid);

    // ... sleep ...
    schedule();
  }

  // ... set the new owner of mutex
  set_current_state(TASK_RUNNING);
  wq.owner = p;

  printk("MOKER[%d][PIP] enqueue task\n", p->pid);
  return;

}

/**
 * Method used to a task unlock the resource
 */
void
unlock_pip_mutex (void) {

  struct task_struct* p = current;
  struct task_struct* t = NULL;
  bool scheduler_up = false;

  t = dequeue_pip_mutex_task();
  printk("MOKER[%d][PIP][DEQ] dequeue task\n", p->pid);

  if ( wq.owner_prio_change ) {
    scheduler_up = true;
  }

  if ( scheduler_up ) {
    struct sched_param param;
    param.sched_priority = wq.owner_original_prio;
    sched_setscheduler(p, wq.owner_original_policy, &param);
    printk("MOKER[%d][PIP][DEQ] update scheduler\n", p->pid);
  }

  // ... update wait queue params
  raw_spin_lock(&wq.lock);

  wq.owner = NULL;
  wq.owner_prio_change = false;
  wq.owner_original_prio = 0;
  wq.owner_original_policy = 0;

  printk("MOKER[%d][PIP][DEQ] reset params\n", p->pid);

  raw_spin_unlock(&wq.lock);

  atomic_set(&wq.flag, 0);

  if ( t ) {
    if ( !wake_up_process(t) ) {
      printk(KERN_ERR "BUG: wake up process failed: %d\n", p->pid);
    }
  }

  return;

}

/**
 * Method to add a new task to waiting queue
 */
int
enqueue_pip_mutex_task (struct task_struct* p) {

  struct pip_mutex_node* pos = NULL;
  struct pip_mutex_node* t = kmalloc(sizeof(struct pip_mutex_node), GFP_KERNEL);
  bool inserted = false;

  if ( !t ) {
    return -1;
  }

  // ... set task to new node ...
  t->task = p;

  // ... add new node to list ...
  raw_spin_lock(&wq.lock);

  list_for_each_entry(pos, &wq.tasks, node) {
    if ( p->prio < pos->task->prio ) {
      list_add_tail(&t->node, &pos->node);
      inserted = true;
      break;
    }
  }

  // ... if new task is the higest priority ...
  if ( !inserted ) {
    list_add_tail(&t->node, &wq.tasks);
  }

  raw_spin_unlock(&wq.lock);

  return 0;

}

/**
 * Method to remove a task from waiting queue
 */
struct task_struct*
dequeue_pip_mutex_task (void) {

  struct task_struct* p = NULL;
  struct pip_mutex_node* t = NULL;

  raw_spin_lock(&wq.lock);

  if ( !list_empty(&wq.tasks) ) {

    // ... get first element of array, because is order by prio ...
    t = list_first_entry(&wq.tasks, struct pip_mutex_node, node);

    // ... point to task ...
    p = t->task;

    // ... remove node from list ...
    list_del(&t->node);

  }

  raw_spin_unlock(&wq.lock);

  // ... clean node ...
  if ( t ) {
    kfree(t);
  }

  return p;

}
