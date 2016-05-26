/*
 * (c) 1997-2013 Daniel Kubec <niel@rtfm.cz>
 *
 * This software may be freely distributed and used according to the terms
 * of the GNU Lesser General Public License.
 */

#ifndef __MPM_TASK_H__
#define __MPM_TASK_H__

#include <ctypes/lib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

extern volatile sig_atomic_t sig_request_term;
extern volatile sig_atomic_t sig_request_hup;
extern volatile sig_atomic_t sig_request_rt;
extern volatile sig_atomic_t sig_request_usr1;

#define TASK_NAME_SEP '_'

struct task;

void task_set_id(struct task *, int id);
void task_set_name(struct task *, const char *name);
void task_set_name_args(struct task *, const char *arg);

extern struct task *idle;

#endif/*__MPM_TASK_H__*/
