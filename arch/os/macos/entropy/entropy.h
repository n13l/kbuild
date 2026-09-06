
#ifndef OS_MACOS_ENTROPY_H
#define OS_MACOS_ENTROPY_H

#include <stddef.h>

#include <arch/os/macos/entropy/hooks.h>

long _sys_getrandom(void *buf, size_t len, unsigned int flags);

int _entropy_is_source_fd(int fd);

#endif
