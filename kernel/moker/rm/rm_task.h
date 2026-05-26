#ifndef __RM_TASK_H_
#define __RM_TASK_H_

#include <linux/rbtree.h>

struct sched_rm_entity {
	unsigned long long period;
	struct rb_node node;
};

#endif
