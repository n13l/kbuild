
#include <stdarg.h>

#include <arch/os/linux/io/io.h>


long
_sys_read(int fd, void *buf, size_t len)
{
	return _syscall3(SYS_read, fd, buf, len);
}

long
_sys_write(int fd, const void *buf, size_t len)
{
	return _syscall3(SYS_write, fd, buf, len);
}

long
_sys_open(const char *path, int flags, int mode)
{
	return _syscall4(SYS_openat, AT_FDCWD, path, flags, mode);
}

long
_sys_close(int fd)
{
	return _syscall1(SYS_close, fd);
}

long
_sys_readlink(const char *path, char *buf, size_t size)
{
	return _syscall4(SYS_readlinkat, AT_FDCWD, path, buf, size);
}

long
_sys_access(const char *path, int mode)
{
	return _syscall4(SYS_faccessat, AT_FDCWD, path, mode, 0);
}

long
_sys_execve(const char *path, char *const argv[], char *const envp[])
{
	return _syscall3(SYS_execve, path, argv, envp);
}

long
_sys_stat(const char *path, struct stat *st)
{
	return _syscall4(SYS_newfstatat, AT_FDCWD, path, st, 0);
}

long
_sys_sendto(int fd, const void *buf, size_t len, int flags, const void *addr,
            unsigned int addrlen)
{
	return _syscall6(SYS_sendto, fd, (long)buf, (long)len, flags,
	                 (long)addr, addrlen);
}

long
_sys_recvfrom(int fd, void *buf, size_t len, int flags, void *addr,
              unsigned int *addrlen)
{
	return _syscall6(SYS_recvfrom, fd, (long)buf, (long)len, flags,
	                 (long)addr, (long)addrlen);
}

long
_sys_getpid(void)
{
	return _syscall0(SYS_getpid);
}

long
_sys_getppid(void)
{
	return _syscall0(SYS_getppid);
}

void
_sys_exit(int status)
{
	for (;;)
		_syscall1(SYS_exit_group, status);
}


void
nolibc_say(int fd, const char *first, ...)
{
	char line[1024];
	const char *piece = first;
	size_t n = 0;
	va_list ap;

	line[0] = '\0';

	va_start(ap, first);
	while (piece) {
		n = xstrlcat(line, sizeof(line) - 1, piece);
		piece = va_arg(ap, const char *);
	}
	va_end(ap);

	line[n++] = '\n';
	_sys_write(fd, line, n);
}
