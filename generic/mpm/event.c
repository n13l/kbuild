/*
 * (c) 1997-2013 Daniel Kubec <niel@rtfm.cz>
 *
 * This software may be freely distributed and used according to the terms
 * of the GNU Lesser General Public License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include <marvin/kernel.h>
#include <marvin/mpm/sched.h>
#include <marvin/mem/stack.h>
#include <marvin/mem/pool.h>

int
event_add_timer(struct task *task, unsigned int sec)
{
        timer_t timer_id = 0;

        /* block timer signal temporarily */
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGRTMIN);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        struct sigaction sa = {
                .sa_flags = SA_SIGINFO,
                .sa_sigaction = sig_handler_timer
        };

        sigemptyset(&sa.sa_mask);
        sigaction(SIGRTMIN, &sa, NULL);

        struct sigevent se = {
                .sigev_notify = SIGEV_SIGNAL,
                .sigev_signo  = SIGRTMIN,
                .sigev_value.sival_ptr = &timer_id
        };

        if (timer_create(CLOCK_MONOTONIC, &se, &timer_id) == -1)
                return -1;

        struct itimerspec it = {
                .it_value.tv_sec     = sec / 1000,
                .it_value.tv_nsec    = (long)(sec % 1000) * (1000000L),
                .it_interval.tv_sec  = sec / 1000,
                .it_interval.tv_nsec = (long)(sec % 1000) * (1000000L)
        };

        timer_settime(timer_id, 0, &it, NULL);

        /* unlock the timer signal, so that timer event can be delivered */
        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        return 0;
}
