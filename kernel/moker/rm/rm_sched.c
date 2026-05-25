#include "../../sched/sched.h"

/*
* RM scheduling class.
* Implements SCHED_RM
*/

static void enqueue_task_rm (struct rq *rq, struct task_struct *p, int flags) {

}

static bool dequeue_task_rm (struct rq *rq, struct task_struct *p, int flags) {

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

  return NULL;
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
