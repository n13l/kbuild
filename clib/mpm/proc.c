
/*
 * (c) 1997-2013 Daniel Kubec <niel@rtfm.cz>
 *
 * This software may be freely distributed and used according to the terms
 * of the GNU Lesser General Public License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <core/lib.h>
#include <mem/stack.h>
#include <mem/pool.h>
#include <core/mpm/task.h>
#include <core/mpm/sched.h>

static struct task *proc;

void
proc_status_report(int id, int status)
{
	if (WIFEXITED(status))
		sys_info("process pid=%d exited, status=%d", id, WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		sys_info("process pid=%d killed by signal %d (%s)", id, WTERMSIG(status), strsignal(WTERMSIG(status)));
	else if (WIFSTOPPED(status))
		sys_info("stopped by signal %d", WSTOPSIG(status));
	else if (WIFCONTINUED(status)) 
		sys_info("continued");
}

void
proc_init(struct task *p)
{
	list_init(&p->usr_exited);
	proc = p;
	sig_disable(SIGHUP);
}

struct task *
proc_self(void)
{
	return proc;
}

void
proc_fini(struct task *p)
{
}

_noreturn static void
proc_child(struct task *p)
{
	/* init per process signal masks and handlers */
	sig_init();
	/* setup case for child processes */
	sig_fork();
	/* move child process to new process group because of signal broadcasts */
	setpgrp();

	proc = p;
	sig_disable(SIGINT);
	sig_disable(SIGTERM);
	sig_disable(SIGPIPE);
	sig_enable(SIGHUP);

	if (p->attrs.cpu_affinity)
		cpu_set_affinity(p->id, CPU_AFFINITY_INDEX, p->cpu);

	task_set_id(p, getpid());
	task_main(p);

	exit(p->status);
}

int
proc_exit(struct task *p)
{
	struct task *parent = (struct task *)p->object.parent;
	return parent ? kill(parent->id, SIGEXIT) : -1;
}

int
proc_fork(struct task *p)
{
        pid_t pid = fork();
	switch (pid) {
	case 0:
		proc_child(p);
	break;
	case -1:
		return -1;
	break;
	default:
        break; 
	}

	p->id = pid;
	return 0;
}

static void
proc_check_user_exit(struct task *p)
{
	/* checking for user defined exit */
	list_for_each(struct task *, c, p->object.childs) {
		if (c->state != TASK_UEXIT)
			continue;

		struct task *d = task_dup(c);
		list_add_tail(&p->usr_exited, &d->object.node);
		task_change_state(p, c, TASK_EXITED);
	}
}

static void
proc_status_check(int id, int status, struct task *p)
{
        if (!WIFEXITED(status) && !WIFSIGNALED(status))
		return;

	void *tmp;
	list_for_each_delsafe(struct task *, c, p->usr_exited, tmp) {
		if (c->id != id)
			continue;

		list_remove(&c->object.node);
		object_free(c);
	}

	list_for_each(struct task *, c, p->object.childs) {
		if (c->id != id)
			continue;

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
			p->tasks--;

		task_change_state(p, c, TASK_EXITED);
		break;
	}
}

int
proc_wait(struct task *p)
{
	if (list_empty(&p->usr_exited) && !p->running)
		return -1;

	if (list_empty(&p->usr_exited))
		sig_enable(SIGHUP);

       	pid_t id = waitpid(-1, &p->wait_status, 0);
	sig_disable(SIGHUP);

	proc_check_user_exit(p);
	if (id < 1)
		return id;

	proc_status_check(id, p->wait_status, p);
       	proc_status_report(id, p->wait_status);

	return 0;
}

int
proc_signal(struct task *p, int signo)
{
        list_for_each(struct task *, c, p->object.childs) {
/*
		list_for_each(struct hook_task_sighook *, h, p->sighook)
			if (h->fn(p, c, signo))
				goto next;
*/
                if (c->state != TASK_RUNNING)
                        continue;

                kill(c->id, signo);
next:
		noop();
        }
        return 0;
}
