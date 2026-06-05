#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "pip_mutex.h"


struct pip_mutex_wq wq;

void pip_mutex_init (void) {
  wq.root = RB_ROOT;

  raw_spin_lock_init(&wq.lock);

  atomic_set(&wq.flag, 0);

  wq.owner = NULL;
}

void pip_mutex_lock () {

  struct task_struct *p = current;
  int scheduler_update = false;

  if ( !atomic_add_unless(&wq.flag, 1, 1) ) {

    // Task can't take mutex

    raw_spin_lock(&wq.lock);

    if (wq.owner) {
      if ( p->rm.period < wq.owner->rm.period ) {
        wq.owner_original_period = wq.owner->rm.period;
        scheduler_update = true;
      }
    }

    raw_spin_unlock(&wq.lock);

    if (scheduler_update) {
      pip_mutex_update_prio(wq.owner, p->rm.period);
    }

    pip_mutex_enqueue(p);

    // Task will wait to get mutex
    for (;;) {
      // ... force task to set status sleep
      set_current_state(TASK_INTERRUPTIBLE);

      // ... check if win mutex/resource
      if ( atomic_add_unless(&wq.flag, 1, 1) ) {
        break;
      }

      // ... sleep ...
      schedule();
    }

    set_current_state(TASK_RUNNING);

  }

  raw_spin_lock(&wq.lock);
  wq.owner = p;
  raw_spin_unlock(&wq.lock);


#ifdef CONFIG_MOKER_TRACING
  moker_trace(MUTEX_LOCK, p, -1);
#endif

}

void pip_mutex_unlock(struct pip_mutex*) {

  struct task_struct *p = current;
  struct task_struct *t = NULL;

  t = pip_mutex_dequeue();

  if(t){
    if(!wake_up_process(t)){
      printk(KERN_ERR "BUG: wake up process failed: %d\n",t->pid);
    }
  }

  atomic_set(&wq.flag,0);

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
    n_period = node->task.rm.period;

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

  return p;
}
