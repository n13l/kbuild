#ifndef OS_IO_H
#define OS_IO_H

/*
 * The system call, whichever platform this is being built for.
 *
 * arch/os/<platform>/io is written per platform because the instruction, the
 * register the number goes in and the numbers themselves all are; what the
 * callers want is one spelling. So they include <arch/os/io.h> and get the
 * _syscall*() and _sys_*() of the platform they are being compiled for, and
 * nothing above this line has to know which that is.
 *
 * The switch is on the compiler's own predefine rather than on CONFIG_PLATFORM,
 * because a header is read by things kbuild does not hand a configuration to
 * — a test compile, an out-of-tree consumer — and __linux__/__APPLE__/_WIN32 is
 * a fact the compiler always has.
 */
#if defined(__APPLE__)
#include <arch/os/macos/io/io.h>
#elif defined(__linux__)
#include <arch/os/linux/io/io.h>
#elif defined(_WIN32)
#include <arch/os/windows/io/io.h>
#else
#error "arch/os: no platform layer here for this system"
#endif

#endif
