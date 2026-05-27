#ifndef __RM_TASK_H_
#define __RM_TASK_H_

struct sched_rm_entity {
	unsigned long long period;
	struct rb_node node;
};

#endif
