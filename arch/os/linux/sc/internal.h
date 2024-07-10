
#ifndef OS_LINUX_SC_INTERNAL_H
#define OS_LINUX_SC_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include <ucontext.h>

#include <arch/os/linux/io/io.h>
#include <arch/os/linux/scan/scan.h>
#include <arch/os/linux/sc/sc.h>

#define SC_MISS_MAX		64u

struct sc_site {
	uintptr_t	pc;
	uint32_t	nr;
	uint32_t	miss;
	uint8_t		hit;
	uint8_t		prot;
	uint8_t		armed;
};

extern struct sc_site	*sc_tab;
extern unsigned int	sc_tab_n;
extern unsigned int	sc_tab_max;
extern struct sc_cfg	sc_conf;
extern size_t		sc_pagesize;
extern unsigned int	sc_image_n;

void sc_tab_sort(void);
struct sc_site *sc_tab_find(uintptr_t pc);

int  sc_arm_image(uintptr_t lo, uintptr_t hi, int prot,
                  const uint64_t *anchor, unsigned int anchor_n);
int  sc_site_revert(struct sc_site *s);
void sc_disarm_all(void);

void sc_walk_self(void);

#define SC_SIGBIT(n)		(1ul << ((n) - 1))

static inline unsigned long *
sc_uc_mask(void *uc)
{
	return (unsigned long *)&((ucontext_t *)uc)->uc_sigmask;
}

int  sc_text_unlock(void *at, size_t len, int prot);
int  sc_text_lock(void *at, size_t len, int prot);

int  sc_trap_open(void);
void sc_trap_close(void);

int  sc_denied(long nr);

#define SC_TRAMPOLINE_SPAN	16u

extern uintptr_t	sc_restorer;


#if defined(__x86_64__)

#define SC_GREG(uc, r)	(((ucontext_t *)(uc))->uc_mcontext.gregs[(r)])

static inline long
sc_uc_nr(void *uc)
{
	return (long)SC_GREG(uc, REG_RAX);
}

static inline void
sc_uc_args(void *uc, long *arg)
{
	arg[0] = (long)SC_GREG(uc, REG_RDI);
	arg[1] = (long)SC_GREG(uc, REG_RSI);
	arg[2] = (long)SC_GREG(uc, REG_RDX);
	arg[3] = (long)SC_GREG(uc, REG_R10);
	arg[4] = (long)SC_GREG(uc, REG_R8);
	arg[5] = (long)SC_GREG(uc, REG_R9);
}

static inline uintptr_t
sc_uc_pc(void *uc)
{
	return (uintptr_t)SC_GREG(uc, REG_RIP);
}

static inline void
sc_uc_finish(void *uc, long ret, uintptr_t next)
{
	SC_GREG(uc, REG_RAX) = (greg_t)ret;
	SC_GREG(uc, REG_RCX) = (greg_t)next;
	SC_GREG(uc, REG_R11) = SC_GREG(uc, REG_EFL);
	SC_GREG(uc, REG_RIP) = (greg_t)next;
}

#elif defined(__aarch64__)

#define SC_REG(uc, i)	(((ucontext_t *)(uc))->uc_mcontext.regs[(i)])

static inline long
sc_uc_nr(void *uc)
{
	return (long)SC_REG(uc, 8);
}

static inline void
sc_uc_args(void *uc, long *arg)
{
	unsigned int i;

	for (i = 0; i < SC_ARGS; i++)
		arg[i] = (long)SC_REG(uc, i);
}

static inline uintptr_t
sc_uc_pc(void *uc)
{
	return (uintptr_t)((ucontext_t *)uc)->uc_mcontext.pc;
}

static inline void
sc_uc_finish(void *uc, long ret, uintptr_t next)
{
	SC_REG(uc, 0) = (unsigned long long)ret;
	((ucontext_t *)uc)->uc_mcontext.pc = (unsigned long long)next;
}

#else
#error "arch/os/linux/sc: this machine has no dispatcher here"
#endif

#endif
