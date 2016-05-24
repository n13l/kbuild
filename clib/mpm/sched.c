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
#include <sys/stat.h>

#include <core/lib.h>
#include <core/mpm/sched.h>
#include <core/mpm/task.h>
#include <mem/stack.h>
#include <mem/pool.h>

void
sched_init(struct sched *sched)
{
	/* disable interrupt requests */
	irq_disable();

	sys_info("cpu.affinity: %s", sched->cpu_affinity ? "yes" : "no" );
	sys_info("cpu.model: %s", cpu_model_names[sched->cpu_model]);
	sys_info("cpu.page.size: %d", CPU_PAGE_SIZE);

	/* setup default interrupts */
	sig_enable(SIGHUP);
	sig_enable(SIGTERM);
	sig_enable(SIGINT);
	sig_enable(SIGSTOP);
	sig_enable(SIGCONT);
}

int
sched_wait(struct sched *sched)
{
	return 0;
}

void
sched_fini(struct sched *sched)
{
}
