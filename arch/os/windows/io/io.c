#ifndef _WIN32_WINNT
#define _WIN32_WINNT	0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdarg.h>
#include <errno.h>

#include <winsock2.h>
#include <windows.h>
#include <winioctl.h>

#include <arch/os/windows/io/io.h>


#ifndef ERROR_NOT_A_REPARSE_POINT
#define ERROR_NOT_A_REPARSE_POINT	4390L
#endif

int
io_errno(unsigned long err)
{
	switch (err) {
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_NAME:
	case ERROR_BAD_PATHNAME:
	case ERROR_INVALID_DRIVE:
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_NET_NAME:
		return ENOENT;
	case ERROR_TOO_MANY_OPEN_FILES:
		return EMFILE;
	case ERROR_ACCESS_DENIED:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
	case ERROR_PRIVILEGE_NOT_HELD:
		return EACCES;
	case ERROR_INVALID_HANDLE:
		return EBADF;
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return ENOMEM;
	case ERROR_WRITE_PROTECT:
		return EROFS;
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_DISK_FULL:
		return ENOSPC;
	case ERROR_NOT_SUPPORTED:
	case ERROR_CALL_NOT_IMPLEMENTED:
		return ENOSYS;
	case ERROR_FILE_EXISTS:
	case ERROR_ALREADY_EXISTS:
		return EEXIST;
	case ERROR_INVALID_PARAMETER:
	case ERROR_NOT_A_REPARSE_POINT:
		return EINVAL;
	case ERROR_BROKEN_PIPE:
	case ERROR_NO_DATA:
		return EPIPE;
	case ERROR_DIR_NOT_EMPTY:
		return ENOTEMPTY;
	case ERROR_BAD_EXE_FORMAT:
		return ENOEXEC;
	case ERROR_FILENAME_EXCED_RANGE:
		return ENAMETOOLONG;
	case ERROR_DIRECTORY:
		return ENOTDIR;
	case ERROR_OPERATION_ABORTED:
		return EINTR;
	case ERROR_NO_UNICODE_TRANSLATION:
		return EILSEQ;

	case WSAEINTR:		return EINTR;
	case WSAEBADF:		return EBADF;
	case WSAEACCES:		return EACCES;
	case WSAEFAULT:		return EFAULT;
	case WSAEINVAL:		return EINVAL;
	case WSAEMFILE:		return EMFILE;
	case WSAEWOULDBLOCK:	return EAGAIN;
	case WSAEINPROGRESS:	return EINPROGRESS;
	case WSAEALREADY:	return EALREADY;
	case WSAENOTSOCK:	return ENOTSOCK;
	case WSAEDESTADDRREQ:	return EDESTADDRREQ;
	case WSAEMSGSIZE:	return EMSGSIZE;
	case WSAEPROTOTYPE:	return EPROTOTYPE;
	case WSAENOPROTOOPT:	return ENOPROTOOPT;
	case WSAEPROTONOSUPPORT: return EPROTONOSUPPORT;
	case WSAEOPNOTSUPP:	return EOPNOTSUPP;
	case WSAEAFNOSUPPORT:	return EAFNOSUPPORT;
	case WSAEADDRINUSE:	return EADDRINUSE;
	case WSAEADDRNOTAVAIL:	return EADDRNOTAVAIL;
	case WSAENETDOWN:	return ENETDOWN;
	case WSAENETUNREACH:	return ENETUNREACH;
	case WSAENETRESET:	return ENETRESET;
	case WSAECONNABORTED:	return ECONNABORTED;
	case WSAECONNRESET:	return ECONNRESET;
	case WSAENOBUFS:	return ENOBUFS;
	case WSAEISCONN:	return EISCONN;
	case WSAENOTCONN:	return ENOTCONN;
	case WSAETIMEDOUT:	return ETIMEDOUT;
	case WSAECONNREFUSED:	return ECONNREFUSED;
	case WSAEHOSTUNREACH:	return EHOSTUNREACH;
	case WSANOTINITIALISED:	return ENOSYS;
	default:
		return EIO;
	}
}

static long
io_fail(void)
{
	return -(long)io_errno(GetLastError());
}

static long
io_fail_wsa(void)
{
	return -(long)io_errno((unsigned long)WSAGetLastError());
}


static HANDLE
io_handle(int fd)
{
	switch (fd) {
	case 0:
		return GetStdHandle(STD_INPUT_HANDLE);
	case 1:
		return GetStdHandle(STD_OUTPUT_HANDLE);
	case 2:
		return GetStdHandle(STD_ERROR_HANDLE);
	default:
		return (HANDLE)(intptr_t)fd;
	}
}

static int
io_fd(HANDLE h)
{
	return (int)(intptr_t)h;
}


#define IO_WPATH_STACK	512

struct io_wpath {
	wchar_t	*p;
	wchar_t	buf[IO_WPATH_STACK];
};

static int
io_wpath_open(struct io_wpath *w, const char *path)
{
	int n;

	w->p = w->buf;
	n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
	if (n <= 0)
		return -(int)io_errno(GetLastError());

	if (n > IO_WPATH_STACK) {
		w->p = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)n * sizeof(wchar_t));
		if (!w->p) {
			w->p = w->buf;
			return -ENOMEM;
		}
	}

	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
	                        w->p, n) <= 0)
		return -(int)io_errno(GetLastError());
	return 0;
}

static void
io_wpath_close(struct io_wpath *w)
{
	if (w->p != w->buf)
		HeapFree(GetProcessHeap(), 0, w->p);
	w->p = w->buf;
}

static long
io_utf8(const wchar_t *s, int len, char *buf, size_t size)
{
	int cap = size > 0x7fffffff ? 0x7fffffff : (int)size;
	int n;

	if (!len || !cap)
		return 0;

	n = WideCharToMultiByte(CP_UTF8, 0, s, len, buf, cap, NULL, NULL);
	if (n > 0)
		return n;

	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		int need = WideCharToMultiByte(CP_UTF8, 0, s, len, NULL, 0,
		                               NULL, NULL);
		char *tmp;
		int i;

		if (need <= 0)
			return io_fail();
		tmp = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)need);
		if (!tmp)
			return -ENOMEM;
		WideCharToMultiByte(CP_UTF8, 0, s, len, tmp, need, NULL, NULL);
		for (i = 0; i < cap; i++)
			buf[i] = tmp[i];
		HeapFree(GetProcessHeap(), 0, tmp);
		return cap;
	}
	return io_fail();
}


long
_sys_read(int fd, void *buf, size_t len)
{
	DWORD got = 0;

	if (len > 0x7fffffff)
		len = 0x7fffffff;

	if (ReadFile(io_handle(fd), buf, (DWORD)len, &got, NULL))
		return (long)got;

	switch (GetLastError()) {
	case ERROR_BROKEN_PIPE:
	case ERROR_HANDLE_EOF:
		return 0;
	default:
		return io_fail();
	}
}

long
_sys_write(int fd, const void *buf, size_t len)
{
	DWORD put = 0;

	if (len > 0x7fffffff)
		len = 0x7fffffff;

	if (!WriteFile(io_handle(fd), buf, (DWORD)len, &put, NULL))
		return io_fail();
	return (long)put;
}

long
_sys_open(const char *path, int flags, int mode)
{
	struct io_wpath w;
	DWORD access, disp, attr;
	HANDLE h;
	int r;

	switch (flags & (O_RDONLY | O_WRONLY | O_RDWR)) {
	case O_WRONLY:
		access = GENERIC_WRITE;
		break;
	case O_RDWR:
		access = GENERIC_READ | GENERIC_WRITE;
		break;
	default:
		access = GENERIC_READ;
		break;
	}
	if ((flags & O_APPEND) && (access & GENERIC_WRITE))
		access = (access & ~GENERIC_WRITE) |
		         (FILE_GENERIC_WRITE & ~FILE_WRITE_DATA);

	if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
		disp = CREATE_NEW;
	else if ((flags & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC))
		disp = CREATE_ALWAYS;
	else if (flags & O_CREAT)
		disp = OPEN_ALWAYS;
	else if (flags & O_TRUNC)
		disp = TRUNCATE_EXISTING;
	else
		disp = OPEN_EXISTING;

	attr = FILE_FLAG_BACKUP_SEMANTICS;
	attr |= ((flags & O_CREAT) && !(mode & 0200)) ? FILE_ATTRIBUTE_READONLY
	                                              : FILE_ATTRIBUTE_NORMAL;

	if ((r = io_wpath_open(&w, path)) < 0) {
		io_wpath_close(&w);
		return r;
	}

	h = CreateFileW(w.p, access,
	                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                NULL, disp, attr, NULL);
	io_wpath_close(&w);

	if (h == INVALID_HANDLE_VALUE)
		return io_fail();
	return io_fd(h);
}

long
_sys_close(int fd)
{
	HANDLE h = io_handle(fd);
	int type, len = sizeof(type);

	if (getsockopt((SOCKET)(uintptr_t)h, SOL_SOCKET, SO_TYPE,
	               (char *)&type, &len) == 0) {
		if (closesocket((SOCKET)(uintptr_t)h) != 0)
			return io_fail_wsa();
		return 0;
	}

	if (!CloseHandle(h))
		return io_fail();

	if (fd >= 0 && fd <= 2)
		SetStdHandle(fd == 0 ? STD_INPUT_HANDLE :
		             fd == 1 ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE, NULL);
	return 0;
}


#define IO_TAG_MOUNT_POINT	0xA0000003UL
#define IO_TAG_SYMLINK		0xA000000CUL
#define IO_REPARSE_MAX		(16 * 1024)

struct io_reparse {
	ULONG	tag;
	USHORT	data_len;
	USHORT	reserved;
	USHORT	subst_off;
	USHORT	subst_len;
	USHORT	print_off;
	USHORT	print_len;
	union {
		struct {
			ULONG	flags;
			WCHAR	path[1];
		} symlink;
		struct {
			WCHAR	path[1];
		} mount;
	} u;
};

static int
io_is_self_exe(const char *path)
{
	return !xstrcmp(path, "/proc/self/exe");
}

static long
io_readlink_self(char *buf, size_t size)
{
	wchar_t name[MAX_PATH + 1];
	DWORD n;

	n = GetModuleFileNameW(NULL, name, MAX_PATH + 1);
	if (!n || n > MAX_PATH)
		return n ? -ENAMETOOLONG : io_fail();
	return io_utf8(name, (int)n, buf, size);
}

long
_sys_readlink(const char *path, char *buf, size_t size)
{
	struct io_reparse *rp;
	struct io_wpath w;
	const WCHAR *base, *s;
	DWORD got = 0;
	HANDLE h;
	long r;
	int len;

	if (io_is_self_exe(path))
		return io_readlink_self(buf, size);

	if ((r = io_wpath_open(&w, path)) < 0) {
		io_wpath_close(&w);
		return r;
	}
	h = CreateFileW(w.p, 0,
	                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                NULL, OPEN_EXISTING,
	                FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
	                NULL);
	io_wpath_close(&w);
	if (h == INVALID_HANDLE_VALUE)
		return io_fail();

	rp = HeapAlloc(GetProcessHeap(), 0, IO_REPARSE_MAX);
	if (!rp) {
		CloseHandle(h);
		return -ENOMEM;
	}

	if (!DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, rp,
	                     IO_REPARSE_MAX, &got, NULL)) {
		r = io_fail();
		goto out;
	}

	switch (rp->tag) {
	case IO_TAG_SYMLINK:
		base = rp->u.symlink.path;
		break;
	case IO_TAG_MOUNT_POINT:
		base = rp->u.mount.path;
		break;
	default:
		r = -EINVAL;
		goto out;
	}

	if (rp->print_len) {
		s = base + rp->print_off / sizeof(WCHAR);
		len = rp->print_len / sizeof(WCHAR);
	} else {
		s = base + rp->subst_off / sizeof(WCHAR);
		len = rp->subst_len / sizeof(WCHAR);
		if (len >= 4 && s[0] == L'\\' && s[1] == L'?' &&
		    s[2] == L'?' && s[3] == L'\\') {
			s += 4;
			len -= 4;
		}
	}
	r = io_utf8(s, len, buf, size);
out:
	HeapFree(GetProcessHeap(), 0, rp);
	CloseHandle(h);
	return r;
}


static int
io_is_exe_name(const wchar_t *p)
{
	const wchar_t *dot = NULL;
	wchar_t e[4];
	int i;

	for (; *p; p++) {
		if (*p == L'.')
			dot = p;
		else if (*p == L'\\' || *p == L'/')
			dot = NULL;
	}
	if (!dot || !dot[1] || !dot[2] || !dot[3] || dot[4])
		return 0;

	for (i = 0; i < 3; i++)
		e[i] = (dot[i + 1] >= L'A' && dot[i + 1] <= L'Z') ?
		       (wchar_t)(dot[i + 1] + 32) : dot[i + 1];
	e[3] = 0;

	return (e[0] == L'e' && e[1] == L'x' && e[2] == L'e') ||
	       (e[0] == L'c' && e[1] == L'o' && e[2] == L'm') ||
	       (e[0] == L'b' && e[1] == L'a' && e[2] == L't') ||
	       (e[0] == L'c' && e[1] == L'm' && e[2] == L'd');
}

long
_sys_access(const char *path, int mode)
{
	struct io_wpath w;
	DWORD attr;
	long r;

	if ((r = io_wpath_open(&w, path)) < 0) {
		io_wpath_close(&w);
		return r;
	}
	attr = GetFileAttributesW(w.p);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		r = io_fail();
	} else if ((mode & IO_W_OK) && (attr & FILE_ATTRIBUTE_READONLY) &&
	           !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
		r = -EACCES;
	} else if ((mode & IO_X_OK) && !(attr & FILE_ATTRIBUTE_DIRECTORY) &&
	           !io_is_exe_name(w.p)) {
		r = -EACCES;
	} else {
		r = 0;
	}
	io_wpath_close(&w);
	return r;
}


#define IO_EPOCH_DELTA	116444736000000000ULL

static time_t
io_time(FILETIME ft)
{
	unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) |
	                       ft.dwLowDateTime;

	if (t < IO_EPOCH_DELTA)
		return 0;
	return (time_t)((t - IO_EPOCH_DELTA) / 10000000ULL);
}

long
_sys_stat(const char *path, struct stat *st)
{
	BY_HANDLE_FILE_INFORMATION fi;
	struct io_wpath w;
	unsigned int mode;
	DWORD type;
	HANDLE h;
	long r;

	if ((r = io_wpath_open(&w, path)) < 0) {
		io_wpath_close(&w);
		return r;
	}
	h = CreateFileW(w.p, FILE_READ_ATTRIBUTES,
	                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		io_wpath_close(&w);
		return io_fail();
	}

	type = GetFileType(h);
	if (type != FILE_TYPE_DISK) {
		mode = type == FILE_TYPE_CHAR ? S_IFCHR | 0666 :
		       type == FILE_TYPE_PIPE ? S_IFIFO | 0666 : 0;
		fi.dwFileAttributes = 0;
		fi.ftCreationTime.dwLowDateTime = 0;
		fi.ftCreationTime.dwHighDateTime = 0;
		fi.ftLastAccessTime = fi.ftCreationTime;
		fi.ftLastWriteTime = fi.ftCreationTime;
		fi.dwVolumeSerialNumber = 0;
		fi.nFileSizeHigh = fi.nFileSizeLow = 0;
		fi.nNumberOfLinks = 1;
		fi.nFileIndexHigh = fi.nFileIndexLow = 0;
	} else if (!GetFileInformationByHandle(h, &fi)) {
		r = io_fail();
		CloseHandle(h);
		io_wpath_close(&w);
		return r;
	} else if (fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		mode = S_IFDIR | 0555;
		if (!(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
			mode |= 0222;
	} else {
		mode = S_IFREG | 0444;
		if (!(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
			mode |= 0222;
		if (io_is_exe_name(w.p))
			mode |= 0111;
	}
	CloseHandle(h);
	io_wpath_close(&w);

	st->st_dev = (__typeof__(st->st_dev))fi.dwVolumeSerialNumber;
	st->st_ino = (__typeof__(st->st_ino))fi.nFileIndexLow;
	st->st_mode = (__typeof__(st->st_mode))mode;
	st->st_nlink = (__typeof__(st->st_nlink))fi.nNumberOfLinks;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_rdev = st->st_dev;
	st->st_size = (__typeof__(st->st_size))
	              (((unsigned long long)fi.nFileSizeHigh << 32) |
	               fi.nFileSizeLow);
	st->st_atime = io_time(fi.ftLastAccessTime);
	st->st_mtime = io_time(fi.ftLastWriteTime);
	st->st_ctime = io_time(fi.ftCreationTime);
	return 0;
}


static size_t
io_quote_len(const char *a)
{
	size_t n = 3;

	for (; *a; a++)
		n += (*a == '"' || *a == '\\') ? 2 : 1;
	return n;
}

static char *
io_quote(char *out, const char *a)
{
	const char *p;
	size_t bs;
	int plain = *a != '\0';

	for (p = a; *p && plain; p++)
		if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\v' ||
		    *p == '"')
			plain = 0;

	if (plain) {
		while (*a)
			*out++ = *a++;
		return out;
	}

	*out++ = '"';
	for (;;) {
		for (bs = 0; *a == '\\'; a++)
			bs++;
		if (!*a) {
			for (bs *= 2; bs; bs--)
				*out++ = '\\';
			break;
		}
		if (*a == '"') {
			for (bs = bs * 2 + 1; bs; bs--)
				*out++ = '\\';
		} else {
			for (; bs; bs--)
				*out++ = '\\';
		}
		*out++ = *a++;
	}
	*out++ = '"';
	return out;
}

static wchar_t *
io_wide(const char *s, int len)
{
	wchar_t *w;
	int n;

	n = MultiByteToWideChar(CP_UTF8, 0, s, len, NULL, 0);
	if (n <= 0)
		return NULL;
	w = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)n * sizeof(wchar_t));
	if (w && MultiByteToWideChar(CP_UTF8, 0, s, len, w, n) <= 0) {
		HeapFree(GetProcessHeap(), 0, w);
		return NULL;
	}
	return w;
}

static void
io_inherit_std(void)
{
	static const DWORD std[] = {
		STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE
	};
	unsigned int i;

	for (i = 0; i < sizeof(std) / sizeof(std[0]); i++) {
		HANDLE h = GetStdHandle(std[i]);

		if (h && h != INVALID_HANDLE_VALUE)
			SetHandleInformation(h, HANDLE_FLAG_INHERIT,
			                     HANDLE_FLAG_INHERIT);
	}
}

long
_sys_execve(const char *path, char *const argv[], char *const envp[])
{
	HANDLE heap = GetProcessHeap();
	wchar_t *app = NULL, *wcmd = NULL, *wenv = NULL;
	char *cmd = NULL, *env = NULL, *o;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	size_t n = 1, i;
	DWORD code;
	long r;

	for (i = 0; argv && argv[i]; i++)
		n += io_quote_len(argv[i]);
	cmd = HeapAlloc(heap, 0, n);
	if (!cmd)
		return -ENOMEM;
	for (o = cmd, i = 0; argv && argv[i]; i++) {
		if (i)
			*o++ = ' ';
		o = io_quote(o, argv[i]);
	}
	*o = '\0';

	if (envp) {
		for (n = 2, i = 0; envp[i]; i++)
			n += xstrlen(envp[i]) + 1;
		env = HeapAlloc(heap, 0, n);
		if (!env) {
			r = -ENOMEM;
			goto out;
		}
		for (o = env, i = 0; envp[i]; i++) {
			const char *e = envp[i];

			while (*e)
				*o++ = *e++;
			*o++ = '\0';
		}
		*o++ = '\0';
		if (o == env + 1)
			*o++ = '\0';
		wenv = io_wide(env, (int)(o - env));
		if (!wenv) {
			r = io_fail();
			goto out;
		}
	}

	wcmd = io_wide(cmd, -1);
	app = io_wide(path, -1);
	if (!wcmd || !app) {
		r = io_fail();
		goto out;
	}

	si.cb = sizeof(si);
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL;
	si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0;
	si.dwXCountChars = si.dwYCountChars = 0;
	si.dwFillAttribute = 0;
	si.dwFlags = STARTF_USESTDHANDLES;
	si.wShowWindow = 0;
	si.cbReserved2 = 0;
	si.lpReserved2 = NULL;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	io_inherit_std();

	if (!CreateProcessW(app, wcmd, NULL, NULL, TRUE,
	                    CREATE_UNICODE_ENVIRONMENT, wenv, NULL, &si, &pi)) {
		DWORD err = GetLastError();
		wchar_t *exe;
		size_t len;

		if (err != ERROR_FILE_NOT_FOUND || io_is_exe_name(app)) {
			r = -(long)io_errno(err);
			goto out;
		}

		for (len = 0; app[len]; len++)
			;
		exe = HeapAlloc(heap, 0, (len + 5) * sizeof(wchar_t));
		if (!exe) {
			r = -ENOMEM;
			goto out;
		}
		for (i = 0; i < len; i++)
			exe[i] = app[i];
		exe[len] = L'.';
		exe[len + 1] = L'e';
		exe[len + 2] = L'x';
		exe[len + 3] = L'e';
		exe[len + 4] = 0;
		HeapFree(heap, 0, app);
		app = exe;

		if (!CreateProcessW(app, wcmd, NULL, NULL, TRUE,
		                    CREATE_UNICODE_ENVIRONMENT, wenv, NULL,
		                    &si, &pi)) {
			r = io_fail();
			goto out;
		}
	}

	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	if (!GetExitCodeProcess(pi.hProcess, &code))
		code = 1;
	CloseHandle(pi.hProcess);
	ExitProcess((UINT)code);

out:
	if (app)
		HeapFree(heap, 0, app);
	if (wcmd)
		HeapFree(heap, 0, wcmd);
	if (wenv)
		HeapFree(heap, 0, wenv);
	if (env)
		HeapFree(heap, 0, env);
	HeapFree(heap, 0, cmd);
	return r;
}


long
_sys_sendto(int fd, const void *buf, size_t len, int flags, const void *addr,
            unsigned int addrlen)
{
	int r;

	if (len > 0x7fffffff)
		len = 0x7fffffff;

	r = sendto((SOCKET)(uintptr_t)(intptr_t)fd, (const char *)buf, (int)len,
	           flags, (const struct sockaddr *)addr, (int)addrlen);
	if (r == SOCKET_ERROR)
		return io_fail_wsa();
	return r;
}

long
_sys_recvfrom(int fd, void *buf, size_t len, int flags, void *addr,
              unsigned int *addrlen)
{
	int alen = addrlen ? (int)*addrlen : 0;
	int r;

	if (len > 0x7fffffff)
		len = 0x7fffffff;

	r = recvfrom((SOCKET)(uintptr_t)(intptr_t)fd, (char *)buf, (int)len,
	             flags, (struct sockaddr *)addr, addrlen ? &alen : NULL);
	if (r == SOCKET_ERROR)
		return io_fail_wsa();
	if (addrlen)
		*addrlen = (unsigned int)alen;
	return r;
}


long
_sys_getpid(void)
{
	return (long)GetCurrentProcessId();
}

struct io_pbi {
	LONG		exit_status;
	PVOID		peb;
	ULONG_PTR	affinity;
	LONG		base_priority;
	ULONG_PTR	pid;
	ULONG_PTR	ppid;
};

__declspec(dllimport) LONG WINAPI
NtQueryInformationProcess(HANDLE process, int cls, PVOID info, ULONG len,
                          PULONG ret);

long
_sys_getppid(void)
{
	struct io_pbi pbi;

	if (NtQueryInformationProcess(GetCurrentProcess(), 0, &pbi, sizeof(pbi),
	                              NULL) < 0)
		return -ENOSYS;
	return (long)pbi.ppid;
}

long
_sys_gettid(void)
{
	return (long)GetCurrentThreadId();
}

long
_sys_gettimeofday(struct timeval *tv)
{
	unsigned long long t;
	FILETIME ft;

	GetSystemTimePreciseAsFileTime(&ft);
	t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	t -= IO_EPOCH_DELTA;

	tv->tv_sec = (__typeof__(tv->tv_sec))(t / 10000000ULL);
	tv->tv_usec = (__typeof__(tv->tv_usec))((t % 10000000ULL) / 10);
	return 0;
}

void
_sys_exit(int status)
{
	for (;;)
		ExitProcess((UINT)status);
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
