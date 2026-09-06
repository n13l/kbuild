#ifndef OS_WINDOWS_COMPAT_SYS_IOCTL_H
#define OS_WINDOWS_COMPAT_SYS_IOCTL_H

/*
 * <sys/ioctl.h> on Windows, for the three requests portable programs actually
 * make, answered over the calls Windows has for each:
 *
 *   TIOCGWINSZ   the console's visible window, from GetConsoleScreenBufferInfo
 *                — the columns and rows a terminal reports, not the size of the
 *                scrollback buffer behind it. ENOTTY when the descriptor is not
 *                a console, which is what a pipe or a file says on Linux too.
 *   FIONBIO      ioctlsocket(), on a socket.
 *   FIONREAD     ioctlsocket(), on a socket.
 *
 * Anything else is ENOTTY. The descriptor is a C runtime one for TIOCGWINSZ
 * (the console is reached through _get_osfhandle) and a SOCKET for the other
 * two, which is what each would be in a program that makes them. Winsock's own
 * FIONBIO and FIONREAD are used as they are; it defines them first.
 */

#include <errno.h>
#include <stdarg.h>
#include <io.h>
#include <winsock2.h>
#include <windows.h>

#define TIOCGWINSZ	0x5413

struct winsize {
	unsigned short	ws_row;
	unsigned short	ws_col;
	unsigned short	ws_xpixel;
	unsigned short	ws_ypixel;
};

static inline int
ioctl(int fd, unsigned long req, ...)
{
	va_list ap;
	void *arg;

	va_start(ap, req);
	arg = va_arg(ap, void *);
	va_end(ap);

	switch (req) {
	case TIOCGWINSZ: {
		CONSOLE_SCREEN_BUFFER_INFO ci;
		struct winsize *ws = (struct winsize *)arg;
		HANDLE h = (HANDLE)_get_osfhandle(fd);

		if (h == INVALID_HANDLE_VALUE ||
		    !GetConsoleScreenBufferInfo(h, &ci)) {
			errno = ENOTTY;
			return -1;
		}
		ws->ws_col = (unsigned short)(ci.srWindow.Right -
		                              ci.srWindow.Left + 1);
		ws->ws_row = (unsigned short)(ci.srWindow.Bottom -
		                              ci.srWindow.Top + 1);
		ws->ws_xpixel = 0;
		ws->ws_ypixel = 0;
		return 0;
	}
	case (unsigned long)FIONBIO:
	case (unsigned long)FIONREAD:
		if (ioctlsocket((SOCKET)(intptr_t)fd, (long)req,
		                (u_long *)arg) == 0)
			return 0;
		errno = WSAGetLastError() == WSAENOTSOCK ? ENOTTY : EINVAL;
		return -1;
	default:
		errno = ENOTTY;
		return -1;
	}
}

#endif
