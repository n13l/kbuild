#include <stdlib.h>
#include <unistd.h>
#include <core/lib.h>
#include <core/mpm/sched.h>

#ifdef CONFIG_HPUX
# include <sys/mpctl.h>
#endif

void
cpu_init(void)
{
#ifdef CONFIG_HPUX
        __cpu_count       = mpctl(MPC_GETNUMSPUS, NULL, NULL);
        __cpu_page_size   = 4096;
#endif
#ifndef CONFIG_WINDOWS
        __cpu_count       = sysconf(_SC_NPROCESSORS_CONF);
        __cpu_page_size   = sysconf(_SC_PAGESIZE);
#else
	__cpu_count       = 1;
	__cpu_page_size   = 4096;
#endif
}

void
cpu_fini(void)
{
}
