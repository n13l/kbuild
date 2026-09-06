
#ifndef OS_LINUX_ENTROPY_HOOKS_H
#define OS_LINUX_ENTROPY_HOOKS_H

#if defined(CONFIG_OS_LINUX_ENTROPY_HOOKS) && \
    defined(CONFIG_OS_LINUX_ENTROPY_HOOKS_HEADER)
#define ENTROPY_HOOKS 1
#include CONFIG_OS_LINUX_ENTROPY_HOOKS_HEADER
#else
#define ENTROPY_HOOKS 0
#endif

#ifndef entropy_hook_fill
#define entropy_hook_fill(buf, len)	do {} while (0)
#endif

#endif
