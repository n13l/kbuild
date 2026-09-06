
#ifndef OS_LINUX_ENTROPY_H
#define OS_LINUX_ENTROPY_H

#include <stddef.h>

#include <arch/os/linux/entropy/hooks.h>

long _sys_getrandom(void *buf, size_t len, unsigned int flags);

#endif
