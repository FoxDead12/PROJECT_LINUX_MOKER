#include "rm_rq.h"

void init_rm_rq (struct rm_rq* rq) {
	INIT_LIST_HEAD(&rq->tasks);
	raw_spin_lock_init(&rq->lock);
	rq->task = NULL;
	rq->nr_running = 0;
}
