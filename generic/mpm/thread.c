#include <core/lib.h>
#include <core/mpm/sched.h>
#include <pthread.h>

struct task *
thread_create(void)
{
	return NULL;
}

int
thread_wait(struct task *p)
{
	return -1;
}

int
thread_signal(struct task *p, int signo)
{
	return -1;
}
