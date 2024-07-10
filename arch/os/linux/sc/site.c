
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/mman.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "internal.h"

#define SC_SITES_DEFAULT	8192u

struct sc_site		*sc_tab;
unsigned int		sc_tab_n;
unsigned int		sc_tab_max;
struct sc_cfg		sc_conf;
size_t			sc_pagesize;
unsigned int		sc_image_n;

static struct scan_site	*sc_found;

static const uint32_t sc_keep[] = {
	(uint32_t)SYS_rt_sigprocmask,
	(uint32_t)SYS_rt_sigaction,
#ifdef SYS_clone3
	(uint32_t)SYS_clone3,
#endif
};

#define SC_KEEP_N	(sizeof(sc_keep) / sizeof(*sc_keep))

static uint32_t		*sc_want;
static unsigned int	sc_want_n;

static int		sc_sorted;

static int		sc_open_done;


static int
by_pc(const void *a, const void *b)
{
	uintptr_t x = ((const struct sc_site *)a)->pc;
	uintptr_t y = ((const struct sc_site *)b)->pc;

	return x < y ? -1 : x > y ? 1 : 0;
}

void
sc_tab_sort(void)
{
	if (sc_tab_n > 1)
		qsort(sc_tab, sc_tab_n, sizeof(*sc_tab), by_pc);
	sc_sorted = 1;
}

struct sc_site *
sc_tab_find(uintptr_t pc)
{
	unsigned int lo = 0, hi = sc_tab_n;

	if (!sc_sorted) {
		unsigned int i;

		for (i = 0; i < sc_tab_n; i++)
			if (sc_tab[i].pc == pc)
				return &sc_tab[i];
		return NULL;
	}

	while (lo < hi) {
		unsigned int mid = lo + (hi - lo) / 2;

		if (sc_tab[mid].pc == pc)
			return &sc_tab[mid];
		if (sc_tab[mid].pc < pc)
			lo = mid + 1;
		else
			hi = mid;
	}
	return NULL;
}


static int
protect(void *at, size_t len, int prot)
{
	uintptr_t a = (uintptr_t)at;
	uintptr_t first = a & ~(uintptr_t)(sc_pagesize - 1);
	uintptr_t last = (a + len - 1) & ~(uintptr_t)(sc_pagesize - 1);

	return (int)_syscall6(SYS_mprotect, (long)first,
	                      (long)(last - first + sc_pagesize), prot,
	                      0, 0, 0) < 0 ? -1 : 0;
}

int
sc_text_unlock(void *at, size_t len, int prot)
{
	return protect(at, len, prot | PROT_WRITE);
}

int
sc_text_lock(void *at, size_t len, int prot)
{
	return protect(at, len, prot);
}


int
sc_arm_image(uintptr_t lo, uintptr_t hi, int prot, const uint64_t *anchor,
             unsigned int anchor_n)
{
	unsigned int n, i, armed = 0;
	struct scan_req r;

	if (!sc_tab || sc_tab_n >= sc_tab_max || hi <= lo)
		return 0;

	memset(&r, 0, sizeof(r));
	r.code = (const uint8_t *)lo;
	r.size = hi - lo;
	r.anchor = anchor;
	r.anchor_n = anchor_n;
	r.want = sc_want;
	r.want_n = sc_want_n;
	r.site = sc_found;
	r.site_max = sc_tab_max - sc_tab_n;

	n = scan_run(&r);

	for (i = 0; i < n; i++) {
		void *at = (void *)(lo + sc_found[i].off);

		if (sc_found[i].nr != SCAN_NR_ANY &&
		    sc_denied((long)sc_found[i].nr))
			continue;
		if (sc_restorer && (uintptr_t)at >= sc_restorer &&
		    (uintptr_t)at < sc_restorer + SC_TRAMPOLINE_SPAN)
			continue;

		if (!scan_is_call(at))
			continue;
		if (sc_text_unlock(at, SCAN_INSN_SIZE, prot))
			continue;

		scan_arm(at);
		scan_sync(at, SCAN_INSN_SIZE);
		sc_text_lock(at, SCAN_INSN_SIZE, prot);

		sc_tab[sc_tab_n].pc = (uintptr_t)at;
		sc_tab[sc_tab_n].nr = sc_found[i].nr;
		sc_tab[sc_tab_n].prot = (uint8_t)prot;
		sc_tab[sc_tab_n].armed = 1;
		sc_tab_n++;
		armed++;
	}

	if (armed)
		sc_image_n++;
	return (int)armed;
}

int
sc_site_revert(struct sc_site *s)
{
	void *at = (void *)s->pc;

	if (!s->armed)
		return 0;

	if (!scan_is_armed(at)) {
		s->armed = 0;
		return 0;
	}

	if (sc_text_unlock(at, SCAN_INSN_SIZE, s->prot))
		return -1;

	scan_disarm(at);
	scan_sync(at, SCAN_INSN_SIZE);
	sc_text_lock(at, SCAN_INSN_SIZE, s->prot);
	s->armed = 0;
	return 0;
}

void
sc_disarm_all(void)
{
	unsigned int i;

	for (i = 0; i < sc_tab_n; i++)
		sc_site_revert(&sc_tab[i]);
}


int
sc_open(const struct sc_cfg *cfg)
{
	unsigned int max;

	if (sc_open_done)
		return 0;
	if (!cfg)
		return -1;

	sc_conf = *cfg;
	sc_pagesize = (size_t)sysconf(_SC_PAGESIZE);
	if (!sc_pagesize || (sc_pagesize & (sc_pagesize - 1)))
		return -1;

	max = cfg->sites_max ? cfg->sites_max : SC_SITES_DEFAULT;
	sc_tab = calloc(max, sizeof(*sc_tab));
	sc_found = calloc(max, sizeof(*sc_found));
	if (!sc_tab || !sc_found)
		goto fail;

	if (cfg->watch && cfg->watch_n) {
		sc_want = calloc(cfg->watch_n + SC_KEEP_N, sizeof(*sc_want));
		if (!sc_want)
			goto fail;
		memcpy(sc_want, cfg->watch,
		       (size_t)cfg->watch_n * sizeof(*sc_want));
		memcpy(sc_want + cfg->watch_n, sc_keep, sizeof(sc_keep));
		sc_want_n = cfg->watch_n + (unsigned int)SC_KEEP_N;
	}

	sc_tab_max = max;

	if (sc_trap_open())
		goto fail;

	sc_open_done = 1;
	return 0;
fail:
	free(sc_tab);
	free(sc_found);
	free(sc_want);
	sc_tab = NULL;
	sc_found = NULL;
	sc_want = NULL;
	sc_want_n = 0;
	sc_tab_max = 0;
	return -1;
}

int
sc_arm_self(void)
{
	if (!sc_open_done)
		return -1;
	if (sc_tab_n)
		return (int)sc_tab_n;

	sc_walk_self();
	sc_tab_sort();

	return (int)sc_tab_n;
}

void
sc_close(void)
{
	if (!sc_open_done)
		return;

	sc_disarm_all();
	sc_trap_close();

	sc_open_done = 0;
	sc_tab_n = 0;
	sc_tab_max = 0;
	sc_image_n = 0;
	sc_sorted = 0;
	free(sc_tab);
	free(sc_found);
	free(sc_want);
	sc_tab = NULL;
	sc_found = NULL;
	sc_want = NULL;
	sc_want_n = 0;
}

unsigned int
sc_armed(void)
{
	unsigned int i, n = 0;

	for (i = 0; i < sc_tab_n; i++)
		n += sc_tab[i].armed;
	return n;
}

unsigned int
sc_images(void)
{
	return sc_image_n;
}

int
sc_running(void)
{
	return sc_open_done;
}
