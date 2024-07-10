
#ifndef OS_LINUX_SC_H
#define OS_LINUX_SC_H

#include <stdint.h>

#define SC_ARGS			6

struct sc_call {
	long		nr;
	long		arg[SC_ARGS];
	long		ret;
	uintptr_t	pc;
};

typedef void (*sc_report_fn)(const struct sc_call *call);

struct sc_cfg {
	const uint32_t	*watch;
	unsigned int	watch_n;
	sc_report_fn	report;
	unsigned int	sites_max;
};

int sc_open(const struct sc_cfg *cfg);

int sc_arm_self(void);

void sc_close(void);

unsigned int sc_armed(void);
unsigned int sc_images(void);
int sc_running(void);

#endif
