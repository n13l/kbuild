#ifndef __SYS_PLATFORM_H__
#define __SYS_PLATFORM_H__

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SHLIB_EX           "dll"

#define HAVE_STRING_H

#define F_LOCK  1
#define F_ULOCK 0
#define F_TLOCK 2
#define F_TEST  3

#define F_GETFL 4


#if defined(__MINGW32__) || defined(__MINGW64__) 

#ifndef S_ISLNK
#define S_ISLNK(X) 0
#endif

#ifndef lstat
#define lstat stat
#endif

#ifndef readlink
#define readlink(file, path, size) do {} while(0)
#endif

int
fsync(int fd);

int
fcntl(int fd, int cmd, ... /* arg */ );

#else

#endif

/*
#ifndef gmtime_r
#define gmtime_r(a,b) gmtime_s(b,a)
#endif
*/

struct tm *
gmtime_r(const time_t *timep, struct tm *result);

struct timespec;

int
posix_clock_gettime(int id, struct timespec *tv);

#endif
