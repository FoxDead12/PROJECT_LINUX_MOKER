#ifndef __SYSCALLS_H
#define __SYSCALLS_H

int do_moker_tracing (unsigned int enable);
int sys_moker_mutex_lock(void);
int sys_moker_mutex_unlock(void);


int sys_moker_pcp_mutex_lock(void);
int sys_moker_pcp_mutex_unlock(void);

#endif
