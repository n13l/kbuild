
#ifndef OS_MACOS_IO_H
#define OS_MACOS_IO_H

#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include <arch/os/str.h>

/*
 * The system call on macOS, and the two things about it that are not Linux.
 *
 * THE NUMBER. On arm64 it goes in x16 rather than x8, and it is the plain BSD
 * number — what <sys/syscall.h> spells SYS_write and the kernel calls 4. On
 * x86-64 it goes in rax as it does everywhere, but with the class in the top
 * byte: Darwin multiplexes several call tables through one instruction and the
 * BSD one is class 2, so the number the instruction carries is 0x2000000 | nr.
 * MACOS_SYS_UNIX below is that constant, and it is the whole of the difference.
 *
 * THE ERROR. Linux returns -errno in the result register and nothing else; the
 * BSD kernels return the error as a positive number and set the carry flag to
 * say that is what it is. This tree is written against the Linux convention —
 * every caller here reads a negative return as the kernel's error — so the
 * carry flag is read with the result and folded back into it. That is why the
 * asm below has a second output and a "cc" clobber and the Linux one does not.
 */
/*
 * THE ARGUMENT REGISTERS ARE NOT PRESERVED. A Darwin system call returns in two
 * registers, not one — x0 and x1 on arm64, rax and rdx on x86-64 — and the
 * second one holds whatever the call left there whether or not the call has a
 * second half to return. Linux returns in one register and leaves the argument
 * registers alone, so the asm there can name them as plain inputs; naming them
 * that way here tells the compiler a lie it will act on. What it costs when it
 * does is not a wrong value, it is a loop: a caller that adds the count it
 * passed to a running total reads the count back out of a register the kernel
 * has since written over, and never arrives at the length it was asked for.
 *
 * So every argument register is an in-out operand below and its output is
 * dropped. That is the whole reason this asm has eight operands and the Linux
 * one has three.
 */
#if defined(__aarch64__)

static inline long
_syscall6(long nr, long a, long b, long c, long d, long e, long f)
{
	register long x16 asm("x16") = nr;
	register long x0 asm("x0") = a;
	register long x1 asm("x1") = b;
	register long x2 asm("x2") = c;
	register long x3 asm("x3") = d;
	register long x4 asm("x4") = e;
	register long x5 asm("x5") = f;
	long err;

	asm volatile ("svc #0x80\n\tcset %1, cs"
	              : "+r"(x0), "=&r"(err), "+r"(x1), "+r"(x2), "+r"(x3),
	                "+r"(x4), "+r"(x5), "+r"(x16)
	              :
	              : "cc", "memory");
	return err ? -x0 : x0;
}

#elif defined(__x86_64__)

#define MACOS_SYS_UNIX		0x2000000L

static inline long
_syscall6(long nr, long a, long b, long c, long d, long e, long f)
{
	register long rdi asm("rdi") = a;
	register long rsi asm("rsi") = b;
	register long rdx asm("rdx") = c;
	register long r10 asm("r10") = d;
	register long r8 asm("r8") = e;
	register long r9 asm("r9") = f;
	unsigned char cf;
	long ret = MACOS_SYS_UNIX | nr;

	asm volatile ("syscall\n\tsetc %1"
	              : "+a"(ret), "=&q"(cf), "+r"(rdi), "+r"(rsi), "+r"(rdx),
	                "+r"(r10), "+r"(r8), "+r"(r9)
	              :
	              : "rcx", "r11", "cc", "memory");
	return cf ? -ret : ret;
}

#else
#error "arch/os/macos/io: no system call instruction is known for this machine"
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

/*
 * This thread's own identifier, and the time of day.
 *
 * Both are here rather than at the caller because both are exactly the kind of
 * fact a platform layer is for. The thread id is thread_selfid(2) on macOS and
 * gettid(2) on Linux — and Darwin has a call it *spells* SYS_gettid which is
 * not this one at all, it reads the thread's effective uid and gid, so a caller
 * reaching for the number it recognised out of <sys/syscall.h> gets the wrong
 * call and no error to say so.
 */
long _sys_gettid(void);
long _sys_gettimeofday(struct timeval *tv);

void _sys_exit(int status) __attribute__((noreturn));


/*
 * A line to a descriptor in one write(2), assembled from its NULL-terminated
 * pieces. The freestanding stand-in for the fprintf() a program with a libc
 * would use for the same job — hence nolibc_, and hence here rather than in
 * str.h: it makes a system call. The string helpers it is built on, and the
 * xstr* dispatch, are in <arch/os/str.h>, included above.
 */
void nolibc_say(int fd, const char *first, ...);

#endif
