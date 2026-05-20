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
  wq_pcp.ceiling_prio = 90;

}

/**
 * Method used to a task lock the resource
 */
int
lock_pcp_mutex (void) {

  struct task_struct* p = current;
  bool scheduler_up = false;

  if ( !atomic_add_unless(&wq_pcp.flag, 1, 1) ) {

    // ... enqueue task ...
    enqueue_pcp_mutex_task(p);
    printk("MOKER[%d][PCP] enqueue\n", p->pid);

    for (;;) {
      // ... force task to set status sleep
      set_current_state(TASK_INTERRUPTIBLE);

      // ... check if win mutex/resource
      if ( atomic_add_unless(&wq_pcp.flag, 1, 1) ) {
        break;
      }

      printk("MOKER[%d][PCP] task will sleep\n", p->pid);

      // ... sleep ...
      schedule();
    }

    set_current_state(TASK_RUNNING);

  }

  wq_pcp.owner = p;

  // ... i think is not necessary but is for a sanity check
  raw_spin_lock(&wq_pcp.lock);

  if ( p->rt_priority < wq_pcp.ceiling_prio ) {
    if ( !wq_pcp.owner_prio_change ) {
      wq_pcp.owner_prio_change = true;
      wq_pcp.owner_original_prio = p->rt_priority;
      printk("MOKER[%d][PCP] store owner original props\n", p->pid);
    }
    scheduler_up = true;
  }

  raw_spin_unlock(&wq_pcp.lock);

  if ( scheduler_up ) {
    struct sched_param param;
    param.sched_priority = wq_pcp.ceiling_prio;
    sched_setscheduler(p, p->policy, &param);
    printk("MOKER[%d][PCP] update scheduler\n", p->pid);
  }

  return 0;

}

/**
 * Method used to a task unlock the resource
 */
int
unlock_pcp_mutex (void) {

  struct task_struct* p = current;
  struct task_struct* t = NULL;
  bool scheduler_up = false;

  t = dequeue_pcp_mutex_task();
  printk("MOKER[%d][PCP][DEQ] dequeue task\n", p->pid);

  // ... update wait queue params
  raw_spin_lock(&wq_pcp.lock);

  if ( wq_pcp.owner_prio_change ) {
    scheduler_up = true;
  }

  wq_pcp.owner = NULL;
  wq_pcp.owner_original_prio = 0;

  printk("MOKER[%d][PCP][DEQ] reset params\n", p->pid);

  raw_spin_unlock(&wq_pcp.lock);

  atomic_set(&wq_pcp.flag, 0);

  if ( scheduler_up ) {
    struct sched_param param;
    param.sched_priority = wq_pcp.owner_original_prio;
    sched_setscheduler(p, p->policy, &param);
    printk("MOKER[%d][PCP][DEQ] update scheduler\n", p->pid);
  }

  if ( t ) {
    if ( !wake_up_process(t) ) {
      printk(KERN_ERR "BUG: wake up process failed: %d\n", p->pid);
    }
  }

  return 0;

}

/**
 * Method to add a new process/task to waiting queue
 */
int
enqueue_pcp_mutex_task (struct task_struct* p) {

  struct pcp_mutex_node* pos = NULL;
  struct pcp_mutex_node* t = kmalloc(sizeof(struct pcp_mutex_node), GFP_KERNEL);
  bool inserted = false;

  if ( !t ) {
    return -1;
  }

  // ... set task to new node ...
  t->task = p;

  // ... add new node to list ...
  raw_spin_lock(&wq_pcp.lock);

  list_for_each_entry(pos, &wq_pcp.tasks, node) {
    if ( p->prio < pos->task->prio ) {
      list_add_tail(&t->node, &pos->node);
      inserted = true;
      break;
    }
  }

  // ... if new task is the higest priority ...
  if ( !inserted ) {
    list_add_tail(&t->node, &wq_pcp.tasks);
  }

  raw_spin_unlock(&wq_pcp.lock);

  return 0;

}

/**
 * Method to remove a task from waiting queue
 */
struct task_struct*
dequeue_pcp_mutex_task (void) {

  struct task_struct* p = NULL;
  struct pcp_mutex_node* t = NULL;

  raw_spin_lock(&wq_pcp.lock);

  if ( !list_empty(&wq_pcp.tasks) ) {

    // ... get first element of array, because is order by prio ...
    t = list_first_entry(&wq_pcp.tasks, struct pcp_mutex_node, node);

    // ... point to task ...
    p = t->task;

    // ... remove node from list ...
    list_del(&t->node);

  }

  raw_spin_unlock(&wq_pcp.lock);

  // ... clean node ...
  if ( t ) {
    kfree(t);
  }

  return p;

}
