#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main(int argc, char *argv[]){
  printf("Start collecting...!\n");

  // .. MOKER syscall to enable tracing ...
  syscall(471,1);
  sleep(20);

  // ... MOKER syscall to disable tracing ...
  syscall(471,0);

  printf("Stop collecting...!\n");
  return 0;
}
