
#include <sys/syscall.h>

#include <arch/os/linux/io/io.h>
#include <arch/os/linux/entropy/entropy.h>


long
_sys_getrandom(void *buf, size_t len, unsigned int flags)
{
	return _syscall3(SYS_getrandom, buf, len, flags);
}
