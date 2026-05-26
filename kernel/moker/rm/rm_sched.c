#include "../../sched/sched.h"

/*
* RM scheduling class.
* Implements SCHED_RM
*/

static void enqueue_task_rm (struct rq *rq, struct task_struct *p, int flags) {
  raw_spin_lock(&rq->rm.lock);
  list_add(&p->rm.node,&rq->rm.tasks);
  rq->rm.task = p;
  rq->rm.nr_running++;
  add_nr_running(rq, 1);
  raw_spin_unlock(&rq->rm.lock);

#ifdef CONFIG_MOKER_TRACING
  moker_trace(ENQUEUE_RQ, p, -1);
#endif

}

static bool dequeue_task_rm (struct rq *rq, struct task_struct *p, int flags) {

  struct sched_rm_entity *t = NULL;

  raw_spin_lock(&rq->rm.lock);
  list_del(&p->rm.node);

  if (list_empty(&rq->rm.tasks)){
    rq->rm.task = NULL;
  } else {
    t = list_first_entry(&rq->rm.tasks,struct sched_rm_entity, node);
    rq->rm.task = container_of(t,struct task_struct, rm);
  }

  rq->rm.nr_running--;
  sub_nr_running(rq, 1);
  raw_spin_unlock(&rq->rm.lock);

#ifdef CONFIG_MOKER_TRACING
  moker_trace(DEQUEUE_RQ, p, -1);
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
  struct task_struct * p = NULL;
  raw_spin_lock(&rq->rm.lock);
  p = rq->rm.task;
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
