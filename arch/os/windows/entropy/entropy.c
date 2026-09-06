#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <errno.h>

#include <windows.h>
#include <bcrypt.h>

#include <arch/os/windows/io/io.h>
#include <arch/os/windows/entropy/entropy.h>

/*
 * getrandom(2), which Windows does not have, over BCryptGenRandom(), which it
 * does. The name and the shape are Linux's because that is what the callers of
 * this tree are written against, and as on macOS two things are made up:
 *
 *   the flags     there are none. The system-preferred generator is seeded
 *                 before any user process runs and never blocks, and there is
 *                 one of it, so the argument is accepted and ignored rather than
 *                 refused.
 *   the length    a ULONG, 32 bits here on every architecture, where getrandom
 *                 takes a size_t. So a request is filled in pieces and a short
 *                 count comes back the way getrandom returns one.
 *
 * BCRYPT_USE_SYSTEM_PREFERRED_RNG and no algorithm handle, so there is nothing
 * to open first or close after, and nothing to fail at startup.
 */
#ifndef BCRYPT_USE_SYSTEM_PREFERRED_RNG
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG	0x00000002
#endif

#define WINDOWS_GENRANDOM_MAX	0x80000000u

long
_sys_getrandom(void *buf, size_t len, unsigned int flags)
{
	unsigned char *p = (unsigned char *)buf;
	size_t done = 0;

	(void)flags;

	while (done < len) {
		size_t n = len - done;

		if (n > WINDOWS_GENRANDOM_MAX)
			n = WINDOWS_GENRANDOM_MAX;

		if (BCryptGenRandom(NULL, p + done, (ULONG)n,
		                    BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
			return done ? (long)done : -EIO;

		done += n;
	}
	return (long)done;
}


/*
 * Whether a descriptor is one of this machine's randomness devices: never.
 * Linux answers with a pair of device numbers and macOS with the st_rdev of two
 * names; Windows has no file a read() draws randomness from — the kernel's
 * generator is \Device\CNG, which takes I/O controls, and nothing opens it by
 * name. So no descriptor is one, which is the truth about this system.
 */
int
_entropy_is_source_fd(int fd)
{
	(void)fd;
	return 0;
}
