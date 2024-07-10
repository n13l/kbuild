#ifndef OS_LINUX_IO_STR_H
#define OS_LINUX_IO_STR_H

#include <stddef.h>

/*
 * Freestanding C-string helpers, and the one rule for reaching them.
 *
 * A program linked -nostdlib (tools/ub, run as a program rather than preloaded)
 * has no <string.h> under it: there is no libc in the image to resolve strlen()
 * against, so the call would be an undefined symbol before the shell ever ran.
 * The bodies below are that missing handful, written out once.
 *
 * Each public name is an xstr* macro that resolves two ways. Where a build has
 * a C library it resolves to the library's own function — which is the tuned,
 * often vectorised one, and worth using. Where a build carries its own way to
 * the kernel instead (CONFIG_OS_LINUX_IO, the same switch the system calls next
 * door answer to) it resolves to the nolibc_ implementation here. So a caller
 * writes xstrlen() and gets the right one for the image it is being built into,
 * and the nolibc_ names stay available for code that wants the own copy outright.
 *
 * strlcpy/strlcat are the exception and always resolve to the nolibc_ copy: they
 * are not standard C, not in every libc, and this tree orders them (dst, size,
 * src) rather than the BSD (dst, src, size), so there is no library name with a
 * matching signature to hand them to. io_say()'s replacement, nolibc_say(), is
 * not here either — it makes a system call, so it lives with them in io.c.
 */

static inline size_t
nolibc_strlen(const char *s)
{
	size_t n = 0;

	while (s[n])
		n++;
	return n;
}

static inline int
nolibc_strcmp(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static inline int
nolibc_strncmp(const char *a, const char *b, size_t n)
{
	size_t i = 0;

	while (i < n && a[i] && a[i] == b[i])
		i++;
	if (i == n)
		return 0;
	return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}

static inline const char *
nolibc_strrchr(const char *s, int c)
{
	const char *last = NULL;

	for (; *s; s++)
		if (*s == (char)c)
			last = s;
	return last;
}

static inline const char *
nolibc_strstr(const char *hay, const char *needle)
{
	size_t n = nolibc_strlen(needle);
	size_t i;

	if (!n)
		return hay;
	for (i = 0; hay[i]; i++)
		if (!nolibc_strncmp(hay + i, needle, n))
			return hay + i;
	return NULL;
}

/* Bounded copy/append, (dst, size, src). Always the own copy — see the header. */
static inline size_t
nolibc_strlcpy(char *dst, size_t size, const char *src)
{
	size_t n = 0;

	if (!size)
		return 0;
	while (src[n] && n + 1 < size) {
		dst[n] = src[n];
		n++;
	}
	dst[n] = '\0';
	return n;
}

static inline size_t
nolibc_strlcat(char *dst, size_t size, const char *src)
{
	size_t n = nolibc_strlen(dst);

	return n + nolibc_strlcpy(dst + n, size > n ? size - n : 0, src);
}

/* Unsigned to decimal into buf. No standard name to defer to; always the own. */
static inline const char *
nolibc_utoa(unsigned long v, char *buf, size_t size)
{
	char tmp[24];
	size_t n = 0, i = 0;

	if (size < 2) {
		if (size)
			buf[0] = '\0';
		return buf;
	}

	do {
		tmp[n++] = (char)('0' + (v % 10));
		v /= 10;
	} while (v && n < sizeof(tmp));

	while (n && i + 1 < size)
		buf[i++] = tmp[--n];
	buf[i] = '\0';
	return buf;
}

#ifdef CONFIG_OS_LINUX_IO		/* own shims: this image carries no libc */

#define xstrlen		nolibc_strlen
#define xstrcmp		nolibc_strcmp
#define xstrncmp	nolibc_strncmp
#define xstrrchr	nolibc_strrchr
#define xstrstr		nolibc_strstr

#else					/* a C library is present: call it */

#include <string.h>

#define xstrlen		strlen
#define xstrcmp		strcmp
#define xstrncmp	strncmp
#define xstrrchr	strrchr
#define xstrstr		strstr

#endif

/* No libc counterpart with this signature — always the own copy. */
#define xstrlcpy	nolibc_strlcpy
#define xstrlcat	nolibc_strlcat

#endif
