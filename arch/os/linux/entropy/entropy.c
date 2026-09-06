
#include <sys/syscall.h>

#include <arch/os/linux/io/io.h>
#include <arch/os/linux/entropy/entropy.h>


long
_sys_getrandom(void *buf, size_t len, unsigned int flags)
{
	return _syscall3(SYS_getrandom, buf, len, flags);
}


#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH	0x1000
#endif

int
_entropy_is_source_fd(int fd)
{
	struct stat st;
	unsigned long rdev;
	unsigned int maj, min;

	if (_syscall4(SYS_newfstatat, fd, (long)"", (long)&st,
	              AT_EMPTY_PATH) < 0)
		return 0;
	if (!S_ISCHR(st.st_mode))
		return 0;

	rdev = (unsigned long)st.st_rdev;
	maj = (unsigned int)((rdev >> 8) & 0xfffUL);
	min = (unsigned int)((rdev & 0xffUL) | ((rdev >> 12) & ~0xffUL));

	switch (maj) {
	case 1:
		return min == 8 || min == 9;
	case 10:
		return min == 183;
	default:
		return 0;
	}
}
