#include <sys/compiler.h>
#include <sys/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#ifdef ARCH_MONOTONIC_CLOCK
timestamp_t
get_timestamp(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (timestamp_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
#else
timestamp_t
get_timestamp(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (timestamp_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif 
