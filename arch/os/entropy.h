#ifndef OS_ENTROPY_H
#define OS_ENTROPY_H

/* The entropy system call of this platform. See <arch/os/io.h>. */
#if defined(__APPLE__)
#include <arch/os/macos/entropy/entropy.h>
#elif defined(__linux__)
#include <arch/os/linux/entropy/entropy.h>
#elif defined(_WIN32)
#include <arch/os/windows/entropy/entropy.h>
#else
#error "arch/os: no platform layer here for this system"
#endif

#endif
