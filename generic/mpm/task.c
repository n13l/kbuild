/*
 * (c) 1997-2013 Daniel Kubec <niel@rtfm.cz>
 *
 * This software may be freely distributed and used according to the terms
 * of the GNU Lesser General Public License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <core/lib.h>
#include <core/mpm/sched.h>
#include <core/mpm/task.h>
#include <mem/stack.h>
#include <mem/pool.h>

struct task *idle;

const char *name = NULL;

int (*hook_idle_configure)(struct task *) = NULL;
int (*hook_idle_timer)(struct task *)     = NULL;

static void timer_handler(int sig, siginfo_t *si, void *u);

static void
task_init_hooks(struct task *p)
{
        list_init(&p->usr_exited);
        list_init(&p->sighook);
        list_init(&p->sigaction);
}

static struct task *
task_find_state(struct task *p, int state)
{
        list_for_each(struct task *, c, p->object.childs)
                if (c->state == state)
                        return c;

        return NULL;
}

static void
task_global_config(void)
{
	if (hook_idle_configure)
		hook_idle_configure(idle);
}

void
task_global_init(struct task_attrs *p)
{
	cpu_init();
	sig_init();
	log_init();

	setproctitle_init(p->argc, p->argv);

        idle = object_alloc_zero(sizeof(*idle));

	task_init(idle, p);
	task_set_name(idle, p->name);

	attr_set(&idle->object, "task.base", p->name);
	attr_set(&idle->object, "task.name", p->name);

	task_global_config();

        u32 tasks     = __cpu_count * p->per_cpu_proc;
	idle->tasks   = p->max_procs && p->max_procs < tasks ? p->max_procs : tasks;
        idle->running = 0;
       	idle->id      = getpid();
/*
	if (hook_idle_timer && p -> idle_timer_msec > 0)
		task_add_timer(p -> idle_timer_msec);
*/
	proc_init(idle);

	sys_info("setting up %d processes on %d cpu(s) system", idle->tasks, __cpu_count);
}

void
task_global_fini(void)
{
	if (idle)
		object_free(idle);

	sig_fini();
	log_fini();
}

static inline int
task_check_pending_rt(struct task *p)
{
	if (!sig_request_rt || p != idle || !hook_idle_timer)
		return 0;

	hook_idle_timer(idle);
	sig_request_rt = 0;
	return 1;
}

static inline int
task_check_pending_hup(struct task *p)
{
	if (!sig_request_hup)
		return 0;

	sys_info("restart requested");
	task_signal(p, SIGHUP);

	if (p == idle)
		task_global_config();

	sig_request_hup = 0;
	return 1;
}

static inline int
task_check_pending_term(struct task *p)
{
        if (!sig_request_term)
		return 0;

	sys_info("shutdown requested");
	task_signal(p, SIGHUP);
	return 1;
}

static inline int
task_global_loop(void)
{
	task_check_pending_hup(idle);
	task_check_pending_rt(idle);

	while (idle->tasks && !sig_request_hup && !sig_request_term && !sig_request_rt) {
		task_balance(idle);
		task_wait(idle);
	}

	if (!idle->tasks)
		return 1;

	return task_check_pending_term(idle);
}

void
task_global_wait(void)
{
	while (!task_global_loop());
	while (idle->running && idle->tasks)
		task_wait(idle);

	/* checking for long run cleanup task */
	while (!list_empty(&idle->usr_exited))
		task_wait(idle);

	sys_info("all processes done.");
}

void
task_init(struct task *p, struct task_attrs *arg)
{
	memcpy(&p->attrs, arg, sizeof(*arg)); 
	p->main = arg->main;
	task_init_hooks(p);
}

void
task_fini(struct task *p)
{
}

struct task *
task_dup(struct task *p)
{
	struct task *c = object_alloc_zero(sizeof(*c));

	c->index = p->index;
	c->cpu   = p->cpu;
	c->level = p->level;
	c->id    = p->id;
	c->state = p->state;
	c->vm    = p->vm;

	c->object.parent = p->object.parent;

	task_init_hooks(c);
	memcpy(&c->attrs, &p->attrs, sizeof(struct task_attrs));
	
	return c;
}

void
task_set_id(struct task *p, int id)
{
	p->id = id;
	log_name(stk_printf("%d", id));
}

void
task_set_name(struct task *p, const char *name)
{
	const char *arg = attr_get(&p->object, "task.name.args");
	attr_set(&p->object, "task.name", name);
	setproctitle("%s.%s", name, arg ? arg :"");
}

void
task_set_name_args(struct task *p, const char *arg)
{
	attr_set(&p->object, "task.name.args", arg);
	setproctitle("%s.%s", attr_get(&p->object, "task.name"), arg);
}

int
task_main(struct task *p)
{
	sys_info("task started pid=%d, index=%.4d/cpu=%.2d", getpid(), p->index, p->cpu);

	//FIXME: block for p->main == NULL
	while(!sig_request_term && !sig_request_hup) 
		if (p->main)
			p->main(p);

	sys_info("task stopped index=%.4d/cpu=%.2d", p->index, p->cpu);
	exit(p->status);
}

void
task_balance(struct task *p)
{
        struct task *c;
	struct task_attrs arg = {
		.vm             = TASK_VM_CLONE,
		.per_cpu_proc   = 0,
		.per_cpu_thread = 0,
		.main           = p->attrs.main
	};

	//FIXME: force down failed task_create 
        while (p->running < p->tasks) if (!(c = task_create(p, &arg)))
		continue;
}

void
task_change_state(struct task *p, struct task *c, int state)
{
	switch (state) {
	case TASK_RUNNING:
		if (c->state != TASK_RUNNING)
			p->running++;
		c->state = state;
	break;
	default:
		if (c->state == TASK_RUNNING)
			p->running--;
		c->state = state;
	break; 
	}
}

struct task *
task_create(struct task *p, struct task_attrs *arg)
{
	struct task *c;
	struct node *n;

	if ((c = task_find_state(p, TASK_UEXIT)))
		goto found;

	if ((c = task_find_state(p, TASK_EXITED)))
		goto found;

       	c = object_alloc_zero(sizeof(*c));
	c->object.parent = (struct object *)p;

	n = &c->object.node;
        list_add_tail(&p->object.childs, n);
	c->index = n->prev != n->next ? ((struct task *)n->prev)->index + 1 : 0;

found:

	c->cpu = c->index % __cpu_count;

	task_init(c, arg);
	if (proc_fork(c) == -1)
		goto failed;

	task_change_state(p, c, TASK_RUNNING);

	return p;
failed:
	if (!c) 
		return NULL;

	list_remove(&c->object.node);
	task_destroy(c);
	
	return NULL;
}

#if 0
static void 
timer_handler(int sig, siginfo_t *si, void *u)
{
	timer_t id = (timer_t)si->si_value.sival_ptr;
	sig_request_rt = 1;
}

int
task_add_timer(unsigned int msec)
{
	timer_t timer_id = 0;

        /* block timer signal temporarily */
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGRTMIN);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	struct sigaction sa = {
		.sa_flags = SA_SIGINFO,
		.sa_sigaction = timer_handler
	};

	sigemptyset(&sa.sa_mask);
	sigaction(SIGRTMIN, &sa, NULL);

	struct sigevent se = {
		.sigev_notify = SIGEV_SIGNAL,
		.sigev_signo  = SIGRTMIN,
		.sigev_value.sival_ptr = &timer_id
	};

	if (timer_create(CLOCK_REALTIME, &se, &timer_id) == -1)
		return -1;

	struct itimerspec it = {
		.it_value.tv_sec     = msec / 1000,
		.it_value.tv_nsec    = (long)(msec % 1000) * (1000000L),
		.it_interval.tv_sec  = msec / 1000,
		.it_interval.tv_nsec = (long)(msec % 1000) * (1000000L)
	};

	timer_settime(timer_id, 0, &it, NULL);

	/* unlock the timer signal, so that timer notification can be delivered */	
	sigprocmask(SIG_UNBLOCK, &mask, NULL);

	return 0;
}

#endif

void
task_destroy(struct task *p)
{
	object_free(p);
}

int
task_wait(struct task *p)
{
	int rv = (p->vm == TASK_VM_SHARED ? thread_wait(p) : proc_wait(p));
	return rv;
}

int
task_signal(struct task *p, int signo)
{
	return (p->vm == TASK_VM_SHARED ? thread_signal(p, signo) : proc_signal(p, signo));
}

int
task_childs(struct task *p)
{
	int count = 0;
	list_for_each(struct task *, c, p->object.childs)
		count++;

	return count;
}
