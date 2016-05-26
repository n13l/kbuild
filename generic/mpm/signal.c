/*
 * (c) 1997-2013 Daniel Kubec <niel@rtfm.cz>
 *
 * This software may be freely distributed and used according to the terms
 * of the GNU Lesser General Public License.
 */

#include <core/lib.h>
#include <mem/stack.h>
#include <core/mpm/sched.h>
#include <core/mpm/task.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <stddef.h>

#ifdef CONFIG_HPUX
#include <ulimit.h>
#include <sys/resource.h>
#endif

/* block mask for other signals while handler runs. */
static sigset_t block_mask;
static sigset_t irq_mask;

volatile sig_atomic_t sig_request_term = 0;
volatile sig_atomic_t sig_request_hup  = 0;
volatile sig_atomic_t sig_request_rt   = 0;
volatile sig_atomic_t sig_request_usr1 = 0;

static void
sig_action_handler(int sig, siginfo_t *si, void *u)
{
        struct task *task = proc_self();

#define DEBUG_SIGNAL
#ifdef DEBUG_SIGNAL
	printf_asynch_safe("signal: %s pid: %d from: %d\n", strsignal(si->si_signo), getpid(), si->si_pid);
#endif
	switch(si->si_signo) {
	case SIGHUP:
		sig_request_hup = 1;
	break;
	case SIGTERM:
	case SIGINT:
		sig_request_term = 1;
	break;
	}

	if (si->si_signo == SIGEXIT) {
	        list_for_each(struct task *, c, task->object.childs) {
			if (c->id != si->si_pid)
				continue;
			task_change_state(task, c, TASK_UEXIT);
		}
	}

	if (list_empty(&task->sigaction))
		return;
/*
	list_for_each(struct hook_task_sigaction *, h, task->sigaction)
		if (h->fn)
			h->fn(sig, si, (siginfo_t *)u);
*/
}

void
sig_init(void)
{
	/* block mask for other signals while handler runs. */
	sigemptyset (&block_mask);
	sigaddset(&block_mask, SIGTERM);
	sigaddset(&block_mask, SIGINT);
	sigaddset(&block_mask, SIGHUP);
	sigaddset(&block_mask, SIGUSR1);
	sigaddset(&block_mask, SIGUSR2);
	sigaddset(&block_mask, SIGEXIT);

	struct sigaction sa = {
		.sa_flags     = SA_SIGINFO,
		.sa_mask      = block_mask,
		.sa_sigaction = sig_action_handler
	};

	/* setup default action for interest signals */
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGHUP,  &sa, NULL);
	sigaction(SIGEXIT, &sa, NULL);

	/* it is usually easier to handle io call than to do 
	 * anything intelligent in a SIGPIPE handler. */
	sig_ignore(SIGPIPE);

	/* block usefull signals before we are ready to use them */
	sig_disable(SIGUSR1);
	sig_disable(SIGUSR2);
	sig_disable(SIGHUP);

	/* initialise atomic signal identifiers */
	sig_request_term = sig_request_hup = sig_request_rt = sig_request_usr1 = 0;
}

void
sig_fini(void)
{
}

void
sig_fork(void)
{
	sig_ignore(SIGTERM);
	sig_ignore(SIGINT);
}

void
sig_ignore(int sig)
{
	signal(sig, SIG_IGN);
}

void
sig_disable(int sig)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, sig);
	sigprocmask(SIG_BLOCK, &mask, NULL);
}

void
sig_enable(int sig)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, sig);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void
irq_enable(void)
{
	sigprocmask(SIG_SETMASK, NULL, &irq_mask);
	sigprocmask(SIG_UNBLOCK, &irq_mask, NULL);
}

void
irq_disable(void)
{
	sigprocmask(SIG_BLOCK, &block_mask, &irq_mask);
}

void
noop(void)
{
}
