#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>
#include <time.h>
#include <errno.h>
#include <posix/darwin/platform.h>

int
posix_clock_gettime(int clock_id, struct timespec *ts)
{
	mach_timespec_t mts;
	static clock_serv_t real_clock = 0;
	static clock_serv_t mono_clock = 0;

	switch (clock_id) {
	case CLOCK_REALTIME:
		if (real_clock == 0)
			host_get_clock_service(mach_host_self(), 
					       CALENDAR_CLOCK, &real_clock);
	
		clock_get_time(real_clock, &mts);
		ts->tv_sec = mts.tv_sec;
		ts->tv_nsec = mts.tv_nsec;
		return 0;
	case CLOCK_MONOTONIC:
		if (mono_clock == 0)
			host_get_clock_service(mach_host_self(), 
					       SYSTEM_CLOCK, &mono_clock);
		
		clock_get_time(mono_clock, &mts);
		ts->tv_sec = mts.tv_sec;
		ts->tv_nsec = mts.tv_nsec;
		return 0;
	default:
		errno = EINVAL;
	return -1;
	}
}
