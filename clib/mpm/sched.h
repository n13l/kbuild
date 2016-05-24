/*
 * (c) 1997-2013 Daniel Kubec <niel@rtfm.cz>
 *
 * This software may be freely distributed and used according to the terms
 * of the GNU Lesser General Public License.
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdlib.h>
#include <sys/types.h>
#include <ctypes/lib.h>
#include <ctypes/list.h>
#include <ctypes/object.h>
#include <signal.h>

__BEGIN_DECLS

enum cpu_model_e {
	CPU_SHARED    = 1,
	CPU_DEDICATED = 2
};

static const char * const cpu_model_names[] = {
	[CPU_SHARED] = "shared", [CPU_DEDICATED] = "dedicated",
};

#define SIGEXIT (SIGRTMIN + 8)

struct task;

enum task_vm {
        TASK_VM_CREATE = 1,
        TASK_VM_CLONE  = 2,
        TASK_VM_SHARED = 3
};

struct sched {
	struct task *task;                 /* root task of this scheluder policy                      */
	enum cpu_model_e cpu_model;
	int  cpu_affinity;
	enum task_vm vm;                   /* memory model for sched's childs tasks                   */
	u32 tasks;                         /* number of child tasks                                   */
};

enum task_state {
	TASK_INIT    = 0,
	TASK_IDLE    = 1,
	TASK_RUNNING = 2,
	TASK_SUSPEND = 4,
	TASK_STOPPED = 5,
	TASK_EXITED  = 6,
	TASK_UEXIT   = 7
};

struct task_attrs {
	enum task_vm vm;
        const char *name;
        const char *ver;
        char **argv;
        int argc;
	u32 cpu_affinity;
	u32 cpu_shared;
	u32 cpu_dedicated;
	u32 per_cpu_proc;
	u32 per_cpu_thread;
	u32 max_procs;
	u32 max_threads;
	u32 max_tasks;
	u32 idle_timer_msec;
	int (*main)(struct task *);
};

struct task {
	struct object object;                 /* instance object -> task tree node */
	struct list usr_exited;               /* user defined acknowledged exit */
	struct list sigaction;                /* sigaction handlers */
	struct list sighook;                  /* hook used when parent send signal to child */
	struct list files;                    /* fds for multiplexing */
	struct list timers;                   /* user defined timers */
	struct task_attrs attrs;
	enum task_state state;
	enum task_vm vm;
	int wait_status;
	int (*main)(struct task *);
	int (*timer)(struct task *);

	u32 status;
	u32 running;
	u32 tasks;
	s32 id;
	u32 index;
	u32 level;
	u32 cpu;                              /* defined for cpu affinity */
};

extern int (*hook_idle_configure)(struct task *);
extern int (*hook_idle_timer)(struct task *);

void
cpu_init(void);

void
cpu_fini(void);

void sched_init(struct sched *);
int sched_wait(struct sched *);
void sched_fini(struct sched *);

void
task_global_init(struct task_attrs *attrs);

void
task_global_fini(void);

void
task_global_wait(void);

void
task_init(struct task *, struct task_attrs *);

void
task_fini(struct task *);

struct task *
task_dup(struct task *p);

void
task_set_name(struct task *p, const char *name);

void
task_set_name_args(struct task *p, const char *arg);

int
task_main(struct task *);

struct task *
task_create(struct task *p, struct task_attrs *arg);

int
task_add_timer(unsigned int secs);

void
task_destroy(struct task *);

void
task_balance(struct task *);

int
task_wait(struct task *);

int
task_signal(struct task *, int);

void
task_change_state(struct task *p, struct task *c, int state);

int
task_childs(struct task *p);

void
proc_init(struct task *p);

void
proc_fini(struct task *p);

struct task *
proc_self(void);

int
proc_exit(struct task *p);

int
proc_fork(struct task *);

int
proc_wait(struct task *p);

int
proc_signal(struct task *p, int);

int
proc_set_affinity(struct task *p);

void
proc_status_report(int id, int status);

int
thread_wait(struct task *p);

int
thread_signal(struct task *p, int signo);

void
setproctitle_init(int argc, char **argv);

void
setproctitle(const char *fmt, ...);

__END_DECLS

#endif
