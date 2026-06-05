#include "defs.h"

void do_work(unsigned long long exec)
{
	unsigned long long i,ten_ns;
	ten_ns=exec/10;
	for(i=0; i<ten_ns; i++){
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
//32

		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);

//64
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);
		asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::); asm volatile  ("nop" ::);

	}
}
int main(int argc, char** argv)
{
	struct sched_param param;
	unsigned long long C, T, O, time0, release;
	unsigned int task_id,njobs,i=0,resource=0;

	struct timespec r;
	task_id=atoi(argv[1]);
	C=(unsigned long long)atoll(argv[2]);
	T=(unsigned long long)atoll(argv[3]);
	O=(unsigned long long)atoll(argv[4])+OFFSET;
	time0 = (unsigned long long)atoll(argv[5]);
	njobs=atoi(argv[6]);
	resource=atoi(argv[7]);

	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(1, &mask);  // CPU 1 — deixas o CPU 0 para o sistema

	if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
		perror("ERROR: sched_setaffinity failed");
		exit(-1);
	}

	printf("Task(%d,%d): before SCHED_RM\n",task_id,getpid());

	param.sched_priority = 0;
	if((sched_setscheduler(0,SCHED_RM,&param)) == -1){
		perror("ERROR:sched_setscheduler failed");
		exit(-1);
	}

	// ... set period of task to SCHED_RM ...
	if (syscall(SYS_MOKER_RM_SET_PERIOD, T) < 0) {
		perror("ERROR: set moker SCHED_RM period failed");
		exit(-1);
	}

	printf("Task(%d,%d): after SCHED_RM\n",task_id,getpid());

	release = time0 + O;
	for(i=0;i<njobs;i++){
		r.tv_sec = release / NSEC_PER_SEC;
		r.tv_nsec = release % NSEC_PER_SEC;


		printf("Task(%d,%d,%d): sleeping until %lld\n",task_id,getpid(),i,release);
		clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME, &r,NULL);
		printf("Task(%d,%d,%d): ready for execution\n",task_id,getpid(),i);

		// ... lock mutex to work ...
		if (resource == 1) {

		}

		do_work(C);

		// ... release mutex to work ...
		if (resource == 1) {

		}


		//computes the next release
		release += T;

	}
	exit(task_id);
	return 0;
}









