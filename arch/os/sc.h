#ifndef OS_SC_H
#define OS_SC_H

/*
 * Arming this process's syscall sites, on the platforms where a process may do
 * that to itself. Linux is the one here: see arch/os/macos/README.md for why
 * there is no macOS half, arch/os/windows/README.md for why there is no Windows
 * half yet, and CONFIG_OS_SC for the symbol that says whether a build has one
 * at all.
 */
#if defined(__linux__)
#include <arch/os/linux/sc/sc.h>
#else
#error "arch/os: this platform has no in-process syscall-site half"
#endif

#endif
