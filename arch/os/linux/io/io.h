
#ifndef OS_LINUX_IO_H
#define OS_LINUX_IO_H

#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <arch/os/linux/io/str.h>


#if defined(__x86_64__)

static inline long
_syscall6(long nr, long a, long b, long c, long d, long e, long f)
{
	register long r10 asm("r10") = d;
	register long r8 asm("r8") = e;
	register long r9 asm("r9") = f;
	long ret;

	asm volatile ("syscall"
	              : "=a"(ret)
	              : "a"(nr), "D"(a), "S"(b), "d"(c),
	                "r"(r10), "r"(r8), "r"(r9)
	              : "rcx", "r11", "memory");
	return ret;
}

#elif defined(__aarch64__)

static inline long
_syscall6(long nr, long a, long b, long c, long d, long e, long f)
{
	register long x8 asm("x8") = nr;
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;
	register long x3 asm("x3") = d;
	register long x4 asm("x4") = e;
	register long x5 asm("x5") = f;

	asm volatile ("svc #0"
	              : "+r"(x0)
	              : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
	              : "memory");
	return x0;
}

#else
#error "arch/os/linux/io: no system call instruction is known for this machine"
#endif

#define _syscall0(nr)			_syscall6((nr), 0, 0, 0, 0, 0, 0)
#define _syscall1(nr, a)		_syscall6((nr), (long)(a), 0, 0, 0, 0, 0)
#define _syscall2(nr, a, b)		_syscall6((nr), (long)(a), (long)(b), \
					          0, 0, 0, 0)
#define _syscall3(nr, a, b, c)		_syscall6((nr), (long)(a), (long)(b), \
					          (long)(c), 0, 0, 0)
#define _syscall4(nr, a, b, c, d)	_syscall6((nr), (long)(a), (long)(b), \
					          (long)(c), (long)(d), 0, 0)


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
void _sys_exit(int status) __attribute__((noreturn));


/*
 * A line to a descriptor in one write(2), assembled from its NULL-terminated
 * pieces. The freestanding stand-in for the fprintf() a program with a libc
 * would use for the same job — hence nolibc_, and hence here rather than in
 * str.h: it makes a system call. The string helpers it is built on, and the
 * xstr* dispatch, are in <arch/os/linux/io/str.h>, included above.
 */
void nolibc_say(int fd, const char *first, ...);

#endif
