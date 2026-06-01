#ifndef __PIP_MUTEX_H_
#define __PIP_MUTEX_H_

/**
 * Struct of mutex
 */
struct pip_mutex_wq {

  atomic_t flag;
  raw_spinlock_t lock;
  struct rb_root root;

  struct task_struct* owner;
  unsigned long long owner_original_period;

};

/**
 * Item will bee stored in tree
 */
struct pip_mutex_node {
	struct rb_node node;
  struct task_struct* task;
};


void pip_mutex_init(void);

void pip_mutex_lock(void);
void pip_mutex_unlock(void);

int pip_mutex_enqueue(struct task_struct *p);
struct task_struct* pip_mutex_dequeue(void);

void pip_mutex_update_prio(struct task_struct* p, unsigned long long new_period);

#endif
