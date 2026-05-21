#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>
#include <errno.h>

int
thread_create (pthread_t* thread, int priority, cpu_set_t* cpuset, void*(*callback_func)(void *)) {

  pthread_attr_t thread_attr;
  struct sched_param sched_p;


  // ... init ...
  pthread_attr_init(&thread_attr);

  // ... set affinity ...
  if ( pthread_attr_setaffinity_np(&thread_attr, sizeof(cpu_set_t), cpuset) ) {
    perror("Falha ao definir afinidade da thread\n");
    return -1;
  }

  // ... set policy of scheduler
  pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);

  sched_p.sched_priority = priority;
  pthread_attr_setschedparam(&thread_attr, &sched_p);

  if ( pthread_create(thread, &thread_attr, callback_func, NULL) != 0 ) {
    perror("Falha ao criar Thread\n");
  }

  return 0;
}

int
thread_work (int it, const char* name) {
  volatile int count = 0;
  for (int i = 0; i < it; i++) {
    count++;
    if (i % (it / 4) == 0) {
      printf("[%s] A processar... (%d/%d)\n", name, i, it);
    }
  }
}

int
resource_lock (const char* name) {
  printf("[KERNEL CALL][%s] -> A pedir o PCP Mutex...\n", name);
  syscall(474);
}

int
resource_unlock (const char* name) {
  printf("[KERNEL CALL][%s] -> A pedir o PCP Mutex...\n", name);
  syscall(475);
}


void* thread_low (void* arg) {
  printf("[LOW][%ld] Iniciou (Prioridade 10). Vai tentar agarrar o Mutex.\n", syscall(SYS_gettid));
  resource_lock("LOW");
  printf("[LOW] Conseguiu o Mutex! A trabalhar na zona critica...\n");
  thread_work(800000000, "LOW");
  printf("[LOW] Terminou o trabalho. Vai largar o Mutex.\n");
  resource_unlock("LOW");
  return NULL;
}

void* thread_med (void* arg) {
  usleep(500000);
  printf("[MED][%ld] Iniciou (Prioridade 50). Nao precisa de mutex, mas vai tentar usar o CPU.\n", syscall(SYS_gettid));
  thread_work(1500000000, "MED");
  printf("[MED] Terminou o seu trabalho pesado.\n");
  return NULL;
}

void* thread_higth (void* arg) {
  sleep(1);
  printf("[HIGH][%ld] Iniciou (Prioridade 90). Precisa urgentemente do Mutex!\n", syscall(SYS_gettid));
  resource_lock("HIGH");
  printf("[HIGH] Conseguiu o Mutex finalmente! A trabalhar de forma super rapida...\n");
  thread_work(100000000, "HIGH");
  printf("[HIGH] Terminou. Vai largar o Mutex.\n");
  resource_unlock("HIGH");
  return NULL;
}

int
main () {

  syscall(471, 1);

  // ... set afinity ...
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);

  pthread_t t_low, t_med, t_high;

  // ... create threads ...
  if ( thread_create(&t_low, 10, &cpuset, thread_low) == -1 ) {
    perror("erro");
    return -1;
  }

  if ( thread_create(&t_med, 50, &cpuset, thread_med) == -1 ) {
    perror("erro");
    return -1;
  }

  if ( thread_create(&t_high, 90, &cpuset, thread_higth) == -1 ) {
    perror("erro");
    return -1;
  }

  // ... wait threads finish ...
  pthread_join(t_low, NULL);
  pthread_join(t_med, NULL);
  pthread_join(t_high, NULL);

  syscall(471, 0);

  return 0;

}

