#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "pip_mutex.h"

#include "../../sched/sched.h"
#include "../trace/trace.h"

struct pip_mutex_wq wq;

void pip_mutex_init (void) {
  wq.root = RB_ROOT;

  raw_spin_lock_init(&wq.lock);

  atomic_set(&wq.flag, 0);

  wq.owner = NULL;
  wq.owner_original_period = -1;
}

void pip_mutex_lock() {

  struct task_struct *p = current;
  int scheduler_update = false;

  if ( atomic_add_unless(&wq.flag, 1, 1) ) {
    wq.owner = p;
    return;
  }

  raw_spin_lock(&wq.lock);
  if (wq.owner) {
    if ( p->rm.period < wq.owner->rm.period ) {
      wq.owner_original_period = wq.owner->rm.period;
      scheduler_update = true;
    }
  }
  raw_spin_unlock(&wq.lock);

  if (scheduler_update) pip_mutex_update_prio(wq.owner, p->rm.period);

  while( !atomic_add_unless(&wq.flag, 1, 1) ){
    set_current_state(TASK_INTERRUPTIBLE);
    pip_mutex_enqueue(p);
    schedule();
  }

#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_LOCK, p, -1);
#endif

}

void pip_mutex_unlock() {

  struct task_struct *p = current;
  struct task_struct *t = NULL;

  // I/owner someone change my period need reset
  if (wq.owner_original_period != -1) {
    pip_mutex_update_prio(wq.owner, wq.owner_original_period);
  }

  raw_spin_lock(&wq.lock);
  wq.owner = NULL;
  wq.owner_original_period = -1;
  raw_spin_unlock(&wq.lock);

  t = pip_mutex_dequeue();

  if (t) {
    if (!wake_up_process(t)) {
      printk(KERN_ERR "BUG: wake up process failed: %d\n",t->pid);
    }
  }

  atomic_set(&wq.flag,0);

#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_UNLOCK, p, -1);
#endif
}

int pip_mutex_enqueue (struct task_struct *p) {

  struct rb_node **new = &wq.root.rb_node;
  struct rb_node *parent = NULL;

  unsigned long long p_period = 0;
  unsigned long long n_period = 0;

  struct pip_mutex_node *n = kmalloc(sizeof(struct pip_mutex_node), GFP_KERNEL);
  n->task = p;

  raw_spin_lock(&wq.lock);

  while ( *new ) {
    struct pip_mutex_node* node = rb_entry(*new, struct pip_mutex_node, node);

    parent = *new;
    p_period = p->rm.period;
    n_period = node->task->rm.period;

    if ( p_period < n_period ) {
      /* left */
      new = &(*new)->rb_left;
    } else {
      /* rigth */
      new = &(*new)->rb_right;
    }
  }

  rb_link_node(&n->node, parent, new);
  rb_insert_color(&n->node, &wq.root);

  raw_spin_unlock(&wq.lock);

#ifdef CONFIG_MOKER_TRACING
  moker_trace(ENQUEUE_WQ, p, -1);
#endif

  return 0;
}

struct task_struct* pip_mutex_dequeue (void) {

  struct rb_node* left = NULL;
  struct pip_mutex_node* node = NULL;
  struct task_struct* p = NULL;

  raw_spin_lock(&wq.lock);

  left = rb_first(&wq.root);

  if ( left ) {
    // ... get struct "sched_rm_entity" position from pointer of rb_node ...
    node = rb_entry(left, struct pip_mutex_node, node);
    p = node->task;
    rb_erase(left, &wq.root);
    kfree(node);
  }

  raw_spin_unlock(&wq.lock);

#ifdef CONFIG_MOKER_TRACING
  if(p) {
    moker_trace(DEQUEUE_WQ, p, -1);
  }
#endif
  return p;
}

void pip_mutex_update_prio(struct task_struct* p, unsigned long long new_period) {
  // ... get run queue of task
  struct rq *rq = task_rq(p);

  dequeue_task_rm(rq, p, DEQUEUE_NOCLOCK);

  p->rm.period = new_period;

  enqueue_task_rm(rq, p, ENQUEUE_NOCLOCK);

}
