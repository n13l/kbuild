#ifndef OS_SCAN_H
#define OS_SCAN_H

/* Where the system call instructions in a run of code are. See <arch/os/io.h>. */
#if defined(__APPLE__)
#include <arch/os/macos/scan/scan.h>
#elif defined(__linux__)
#include <arch/os/linux/scan/scan.h>
#elif defined(_WIN32)
#include <arch/os/windows/scan/scan.h>
#else
#error "arch/os: no platform layer here for this system"
#endif

#endif
