#ifndef __RM_TASK_H_
#define __RM_TASK_H_

struct sched_rm_entity {
	unsigned long long period;
	struct list_head node;
	// struct rb_node node;
};

#endif
