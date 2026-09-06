#ifndef OS_WINDOWS_IO_H
#define OS_WINDOWS_IO_H

#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <arch/os/str.h>

/*
 * The system call on Windows, which is not an instruction a program may make.
 *
 * There is one — `syscall` in ntdll on x86-64, `svc` with the number in the
 * immediate on arm64 — but the numbers behind it are renumbered by every
 * Windows build and documented by none of them, and the only code entitled to
 * know them ships in the same update that changes them: ntdll.dll. So a program
 * that wants the kernel without a C library under it calls kernel32, which is
 * not a C library at all but the system's own interface, stable since NT 3.1,
 * and the layer Microsoft's own CRT is written over. That is what io.c does:
 * one kernel32 or Winsock call per _sys_ name, no msvcrt underneath, and no
 * _syscall0() .. _syscall6() here for anything to reach past it with — a number
 * that is right on this machine is wrong on the next one.
 *
 * Three things the callers of this tree assume and Windows does not have are
 * made up here, and io.c says how at each one:
 *
 *   the descriptor  an int. A kernel object here is a HANDLE, and the int is
 *                   the handle's value (0, 1 and 2 are the standard handles);
 *                   a socket is a SOCKET and its value is the int the same way.
 *   the error       a negative errno, as on Linux, translated from what
 *                   GetLastError() or WSAGetLastError() said (io_errno()).
 *   the path        UTF-8, as every other string in this tree, converted to the
 *                   UTF-16 the wide API takes.
 */

#define IO_F_OK		0
#define IO_X_OK		1
#define IO_W_OK		2
#define IO_R_OK		4

long _sys_read(int fd, void *buf, size_t len);
long _sys_write(int fd, const void *buf, size_t len);
long _sys_open(const char *path, int flags, int mode);
long _sys_close(int fd);
long _sys_readlink(const char *path, char *buf, size_t size);
long _sys_access(const char *path, int mode);
long _sys_execve(const char *path, char *const argv[], char *const envp[]);

long _sys_sendto(int fd, const void *buf, size_t len, int flags,
                 const void *addr, unsigned int addrlen);
long _sys_recvfrom(int fd, void *buf, size_t len, int flags,
                   void *addr, unsigned int *addrlen);
long _sys_stat(const char *path, struct stat *st);

long _sys_getpid(void);
long _sys_getppid(void);

/*
 * This thread's own identifier (GetCurrentThreadId — the kernel's, like
 * gettid(2), not a pseudo-handle), and the time of day (the precise clock, not
 * the scheduler-tick one a timeval would otherwise lie about).
 */
long _sys_gettid(void);
long _sys_gettimeofday(struct timeval *tv);

void _sys_exit(int status) __attribute__((noreturn));

/*
 * The errno a Win32 or Winsock error code stands for, positive. What every
 * _sys_ above negates and returns on failure, exported because a caller holding
 * a GetLastError() of its own needs the same translation. No POSIX counterpart
 * is EIO.
 */
int io_errno(unsigned long err);


/*
 * A line to a descriptor in one write, assembled from its NUL-terminated
 * pieces. The freestanding stand-in for the fprintf() a program with a libc
 * would use — hence nolibc_, and hence here rather than in str.h: it makes a
 * call into the system. The string helpers it is built on are in <arch/os/str.h>.
 */
void nolibc_say(int fd, const char *first, ...);

#endif
