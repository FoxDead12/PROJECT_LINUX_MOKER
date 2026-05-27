#include "../../sched/sched.h"

/*
* RM scheduling class.
* Implements SCHED_RM
*/

static void enqueue_task_rm (struct rq *rq, struct task_struct *p, int flags) {

  // ... create necessary variables ...
  struct rb_node **new = &rq->rm.root.rb_node;
  struct rb_node *parent = NULL;

  unsigned long long p_period = 0;
  unsigned long long n_period = 0;

  // ... block multiple edition ...
  raw_spin_lock(&rq->rm.lock);

  // ... look for position to new task in tree ...
  while ( *new ) {
    struct sched_rm_entity* node = rb_entry(*new, struct sched_rm_entity, node);

    parent = *new;
    p_period = p->rm.period;
    n_period = node->period;

    if ( p_period < n_period ) {
      /* left */
      new = &(*new)->rb_left;
    } else {
      /* rigth */
      new = &(*new)->rb_right;
    }
  }

  // ... will add new item in end of tree "arm" ...
  rb_link_node(&p->rm.node, parent, new);
  rb_insert_color(&p->rm.node, &rq->rm.root);

  rq->rm.nr_running++;
  add_nr_running(rq, 1);

  raw_spin_unlock(&rq->rm.lock);

#ifdef CONFIG_MOKER_TRACING
  moker_trace(ENQUEUE_RQ, p, rq->cpu);
#endif

}

static bool dequeue_task_rm (struct rq *rq, struct task_struct *p, int flags) {

  // ... block multiple edition ...
  raw_spin_lock(&rq->rm.lock);

  rb_erase(&p->rm.node, &rq->rm.root);

  rq->rm.nr_running--;
  sub_nr_running(rq, 1);

  raw_spin_unlock(&rq->rm.lock);

#ifdef CONFIG_MOKER_TRACING
  moker_trace(DEQUEUE_RQ, p, rq->cpu);
#endif

  return true;
}

/*
* Preempt the current task with a newly woken task if needed:
*/
static void wakeup_preempt_rm (struct rq *rq, struct task_struct *p, int flags) {

  switch(rq->donor->policy){
    case SCHED_DEADLINE:
      break;
    case SCHED_FIFO:
    case SCHED_RR:
    case SCHED_NORMAL:
    case SCHED_BATCH:
    case SCHED_IDLE:
    case SCHED_RM:
      resched_curr(rq);
      break;
  }

}

static struct task_struct *pick_task_rm (struct rq *rq, struct rq_flags *rf) {
  struct rb_node* left = NULL;
  struct sched_rm_entity* task_rm = NULL;
  struct task_struct* p = NULL;

  raw_spin_lock(&rq->rm.lock);

  // ... get the most left item in tree is one how has lowest period ...
  left = rb_first(&rq->rm.root);

  if ( left ) {
    // ... get struct "sched_rm_entity" position from pointer of rb_node ...
    task_rm = rb_entry(left, struct sched_rm_entity, node);
    // ... get struct "task_struct" from  pointer in rm struct ...
    p = container_of(task_rm, struct task_struct, rm);
  }

  raw_spin_unlock(&rq->rm.lock);

  return p;
}

static void put_prev_task_rm (struct rq *rq, struct task_struct *p, struct task_struct *next) {
}

static void set_next_task_rm (struct rq *rq, struct task_struct *p, bool first) {
}

static int select_task_rq_rm (struct task_struct *p, int cpu, int flags) {
  return cpu;
}

static void task_tick_rm (struct rq *rq, struct task_struct *p, int queued) {
}

static void prio_changed_rm (struct rq *rq, struct task_struct *p, u64 oldprio) {
}

static void switched_to_rm (struct rq *rq, struct task_struct *p) {
}

static void update_curr_rm (struct rq *rq) {
}

DEFINE_SCHED_CLASS(rm) = {
  .queue_mask = 8,
  .enqueue_task = enqueue_task_rm,
  .dequeue_task = dequeue_task_rm,
  .wakeup_preempt = wakeup_preempt_rm,
  .pick_task = pick_task_rm,
  .put_prev_task = put_prev_task_rm,
  .set_next_task = set_next_task_rm,
  .select_task_rq = select_task_rq_rm,
  .set_cpus_allowed = set_cpus_allowed_common,
  .task_tick = task_tick_rm,
  .prio_changed = prio_changed_rm,
  .switched_to = switched_to_rm,
  .update_curr = update_curr_rm,
};
