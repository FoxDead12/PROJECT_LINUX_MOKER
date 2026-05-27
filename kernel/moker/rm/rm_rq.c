#include "rm_rq.h"

void init_rm_rq (struct rm_rq* rq) {
	// ... init red black tree ...
	rq->root = RB_ROOT;

	// ... init spin lock ...
	raw_spin_lock_init(&rq->lock);

	// ... init count of waiting task in run queue ...
	rq->nr_running = 0;
}
