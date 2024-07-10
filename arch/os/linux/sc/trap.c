
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/syscall.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "internal.h"

#ifndef SA_RESTORER
#define SA_RESTORER	0x04000000u
#endif

struct sc_ksigaction {
	void		*handler;
	unsigned long	flags;
	void		(*restorer)(void);
	unsigned long	mask;
};

static int		sc_have_old;

static struct sc_ksigaction sc_ill;

uintptr_t		sc_restorer;

static __thread int	sc_inside;

int
sc_denied(long nr)
{
	switch (nr) {
#ifdef SYS_clone
	case SYS_clone:
#endif
#ifdef SYS_fork
	case SYS_fork:
#endif
#ifdef SYS_vfork
	case SYS_vfork:
#endif
#ifdef SYS_rt_sigreturn
	case SYS_rt_sigreturn:
#endif
#ifdef SYS_sigaltstack
	case SYS_sigaltstack:
#endif
		return 1;
	default:
		return 0;
	}
}

static int
watched(long nr)
{
	unsigned int i;

	if (!sc_conf.watch || !sc_conf.watch_n)
		return 1;
	for (i = 0; i < sc_conf.watch_n; i++)
		if ((long)sc_conf.watch[i] == nr)
			return 1;
	return 0;
}

static long
mask_change(void *ucv, const struct sc_call *c)
{
	unsigned long *frame = sc_uc_mask(ucv);
	const unsigned long *set = (const unsigned long *)c->arg[1];
	unsigned long *old = (unsigned long *)c->arg[2];
	unsigned long cur = *frame, next;

	if ((size_t)c->arg[3] != sizeof(*frame))
		return -EINVAL;

	if (old)
		*old = cur;
	if (!set)
		return 0;

	switch ((int)c->arg[0]) {
	case SIG_BLOCK:
		next = cur | *set;
		break;
	case SIG_UNBLOCK:
		next = cur & ~*set;
		break;
	case SIG_SETMASK:
		next = *set;
		break;
	default:
		return -EINVAL;
	}

	*frame = next & ~(SC_SIGBIT(SIGILL) | SC_SIGBIT(SIGKILL) |
	                  SC_SIGBIT(SIGSTOP));
	return 0;
}

#define SC_CLONE_CLEAR_SIGHAND	0x100000000ull

static long
clone3_refuse(const struct sc_call *c)
{
	uint64_t *flags = (uint64_t *)c->arg[0];

	if (flags && (size_t)c->arg[1] >= sizeof(*flags))
		*flags &= ~SC_CLONE_CLEAR_SIGHAND;

	return -ENOSYS;
}

static long
ill_disposition(const struct sc_call *c)
{
	const struct sc_ksigaction *act = (const struct sc_ksigaction *)
	                                  c->arg[1];
	struct sc_ksigaction *old = (struct sc_ksigaction *)c->arg[2];

	if ((size_t)c->arg[3] != sizeof(sc_ill.mask))
		return -EINVAL;
	if (old)
		*old = sc_ill;
	if (act)
		sc_ill = *act;
	return 0;
}

static void
foreign(int sig, siginfo_t *si, void *uc)
{
	void *h = sc_ill.handler;

	if (h == (void *)SIG_IGN)
		return;

	if (h && h != (void *)SIG_DFL) {
		if (sc_ill.flags & SA_SIGINFO)
			((void (*)(int, siginfo_t *, void *))h)(sig, si, uc);
		else
			((void (*)(int))h)(sig);
		return;
	}

	_syscall6(SYS_rt_sigaction, sig, (long)&sc_ill, 0,
	          sizeof(sc_ill.mask), 0, 0);
}

static void
sc_trap(int sig, siginfo_t *si, void *ucv)
{
	uintptr_t pc = sc_uc_pc(ucv);
	int saved = errno;
	struct sc_site *s;
	struct sc_call c;

	s = sc_tab_find(pc);
	if (!s) {
		foreign(sig, si, ucv);
		errno = saved;
		return;
	}

	if (!s->armed) {
		errno = saved;
		return;
	}

	c.nr = sc_uc_nr(ucv);
	c.pc = pc;
	sc_uc_args(ucv, c.arg);

#ifdef SYS_clone3
	if (c.nr == SYS_clone3) {
		c.ret = clone3_refuse(&c);
		sc_uc_finish(ucv, c.ret, pc + SCAN_INSN_SIZE);
		errno = saved;
		return;
	}
#endif

	if (c.nr == SYS_rt_sigaction && c.arg[0] == SIGILL) {
		c.ret = ill_disposition(&c);
		sc_uc_finish(ucv, c.ret, pc + SCAN_INSN_SIZE);
		errno = saved;
		return;
	}

	if (c.nr == SYS_rt_sigprocmask) {
		c.ret = mask_change(ucv, &c);
		sc_uc_finish(ucv, c.ret, pc + SCAN_INSN_SIZE);
		errno = saved;
		return;
	}

	if (sc_denied(c.nr) && !sc_site_revert(s)) {
		errno = saved;
		return;
	}

	c.ret = _syscall6(c.nr, c.arg[0], c.arg[1], c.arg[2], c.arg[3],
	                  c.arg[4], c.arg[5]);

	sc_uc_finish(ucv, c.ret, pc + SCAN_INSN_SIZE);

	if (!watched(c.nr)) {
		if (s->nr == SCAN_NR_ANY && !s->hit &&
		    ++s->miss >= SC_MISS_MAX)
			sc_site_revert(s);
	} else {
		s->hit = 1;
		if (sc_conf.report && !sc_inside) {
			sc_inside = 1;
			sc_conf.report(&c);
			sc_inside = 0;
		}
	}

	errno = saved;
}

int
sc_trap_open(void)
{
	struct sigaction sa;

	memset(&sc_ill, 0, sizeof(sc_ill));
	_syscall6(SYS_rt_sigaction, SIGILL, 0, (long)&sc_ill,
	          sizeof(sc_ill.mask), 0, 0);

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = sc_trap;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO | SA_NODEFER;

	if (sigaction(SIGILL, &sa, NULL))
		return -1;
	sc_have_old = 1;

	{
		struct sc_ksigaction ksa;

		memset(&ksa, 0, sizeof(ksa));
		if (_syscall6(SYS_rt_sigaction, SIGILL, 0, (long)&ksa,
		              sizeof(ksa.mask), 0, 0) >= 0 &&
		    (ksa.flags & SA_RESTORER) && ksa.restorer)
			sc_restorer = (uintptr_t)ksa.restorer;
	}

	return 0;
}

void
sc_trap_close(void)
{
	if (sc_have_old) {
		_syscall6(SYS_rt_sigaction, SIGILL, (long)&sc_ill, 0,
		          sizeof(sc_ill.mask), 0, 0);
		sc_have_old = 0;
	}
}
