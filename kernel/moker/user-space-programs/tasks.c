#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h> // Necessário para a função syscall()
#include <errno.h>       // Necessário para ver os erros de permissão

// ====================================================================
// PONTES PARA O TEU KERNEL MODULE
// ====================================================================
void chamar_lock_do_kernel() {
  printf("[KERNEL CALL] -> A pedir o PIP Mutex...\n");
  syscall(472);
}

void chamar_unlock_do_kernel() {
  printf("[KERNEL CALL] -> A libertar o PIP Mutex...\n");
  syscall(473);
}

// ====================================================================
// THREADS DE TESTE
// ====================================================================

void* thread_low(void* arg) {
  printf("[LOW] Iniciou (Prioridade 10). Vai tentar agarrar o Mutex.\n");
  chamar_lock_do_kernel();

  printf("[LOW] Conseguiu o Mutex! A trabalhar na zona critica...\n");
  // Simula trabalho longo. Durante este tempo as outras vão acordar.
  for (int i = 0; i < 5; i++) {
      printf("[LOW] A trabalhar... %d/5\n", i+1);
      sleep(1);
  }

  printf("[LOW] Terminou o trabalho. Vai largar o Mutex.\n");
  chamar_unlock_do_kernel();
  return NULL;
}

void* thread_med(void* arg) {
  // Atraso para garantir que a LOW arranca primeiro
  usleep(500000);

  printf("[MED] Iniciou (Prioridade 50). Nao precisa de mutex, mas vai tentar usar o CPU.\n");

  // Simula trabalho intensivo no CPU (sem dormir) para tentar roubar o CPU à LOW
  volatile int contador = 0;
  for (int i = 0; i < 2000000000; i++) {
      contador++;
  }

  printf("[MED] Terminou o seu trabalho pesado.\n");
  return NULL;
}

void* thread_high(void* arg) {
  // Atraso para garantir que a MED arranca antes de nós
  sleep(1);

  printf("[HIGH] Iniciou (Prioridade 90). Precisa urgentemente do Mutex!\n");
  chamar_lock_do_kernel(); // Aqui o teu PIP no kernel deve fazer o BOOST à LOW!

  printf("[HIGH] Conseguiu o Mutex finalmente! A trabalhar de forma super rapida...\n");
  usleep(500000); // Trabalho rápido

  printf("[HIGH] Terminou. Vai largar o Mutex.\n");
  chamar_unlock_do_kernel();
  return NULL;
}

// ====================================================================
// MAIN
// ====================================================================
int main() {
  pthread_t t_low, t_med, t_high;
  pthread_attr_t attr_low, attr_med, attr_high;
  struct sched_param param_low, param_med, param_high;
  int erro;

  // Inicializa o Mutex no Kernel
  syscall(471, 1);
  printf("--- INICIO DO TESTE PIP MUTEX ---\n");

  // Inicializa atributos das threads
  pthread_attr_init(&attr_low);
  pthread_attr_init(&attr_med);
  pthread_attr_init(&attr_high);

  // Obriga as threads a usarem a política e prioridade que definirmos aqui
  pthread_attr_setinheritsched(&attr_low, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setinheritsched(&attr_med, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setinheritsched(&attr_high, PTHREAD_EXPLICIT_SCHED);

  // Define Política para Tempo Real (FIFO)
  pthread_attr_setschedpolicy(&attr_low, SCHED_FIFO);
  pthread_attr_setschedpolicy(&attr_med, SCHED_FIFO);
  pthread_attr_setschedpolicy(&attr_high, SCHED_FIFO);

  // Define as Prioridades (Lembrar: em User Space no FIFO, 99 é o máximo)
  param_low.sched_priority = 10;
  pthread_attr_setschedparam(&attr_low, &param_low);

  param_med.sched_priority = 50;
  pthread_attr_setschedparam(&attr_med, &param_med);

  param_high.sched_priority = 90;
  pthread_attr_setschedparam(&attr_high, &param_high);

  // Criar as threads com verificações de erro
  erro = pthread_create(&t_low, &attr_low, thread_low, NULL);
  if (erro != 0) {
      errno = erro;
      perror("[ERRO] Falha ao criar Thread LOW (Nao te esqueceste do sudo?)");
      exit(EXIT_FAILURE);
  }

  erro = pthread_create(&t_med, &attr_med, thread_med, NULL);
  if (erro != 0) {
      errno = erro;
      perror("[ERRO] Falha ao criar Thread MED");
      exit(EXIT_FAILURE);
  }

  erro = pthread_create(&t_high, &attr_high, thread_high, NULL);
  if (erro != 0) {
      errno = erro;
      perror("[ERRO] Falha ao criar Thread HIGH");
      exit(EXIT_FAILURE);
  }

  // Esperar que todas terminem
  pthread_join(t_low, NULL);
  pthread_join(t_med, NULL);
  pthread_join(t_high, NULL);

  printf("--- FIM DO TESTE PIP MUTEX ---\n");

  // Limpa/Destrói o Mutex no Kernel
  syscall(471, 0);

  return 0;
}
