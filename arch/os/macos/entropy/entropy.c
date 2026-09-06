#include <sys/syscall.h>

#include <arch/os/macos/io/io.h>
#include <arch/os/macos/entropy/entropy.h>

/*
 * getrandom(2), which macOS does not have, over getentropy(2), which it does.
 *
 * The name and the shape are Linux's because that is what the callers of this
 * tree are written against, and two things have to be made up to keep them:
 *
 *   the flags     there are none. GRND_NONBLOCK and GRND_RANDOM are choices
 *                 about a pool that is not how this kernel is built; getentropy
 *                 never blocks and has one source. The argument is accepted and
 *                 ignored rather than refused, so a caller that passes what it
 *                 passes on Linux gets randomness rather than -EINVAL.
 *
 *   the length    getentropy takes at most 256 bytes and fails the whole call
 *                 for one byte more, where getrandom takes any length. So the
 *                 loop below is the difference, and a short count is returned
 *                 the way getrandom returns one: what was filled, or the
 *                 kernel's error if nothing was.
 */
#define MACOS_GETENTROPY_MAX	256u

long
_sys_getrandom(void *buf, size_t len, unsigned int flags)
{
	unsigned char *p = (unsigned char *)buf;
	size_t done = 0;

	(void)flags;

	while (done < len) {
		size_t n = len - done;
		long r;

		if (n > MACOS_GETENTROPY_MAX)
			n = MACOS_GETENTROPY_MAX;

		r = _syscall2(SYS_getentropy, p + done, n);
		if (r < 0)
			return done ? (long)done : r;

		done += n;
	}
	return (long)done;
}


/*
 * Whether a descriptor is one of this machine's randomness devices.
 *
 * On Linux the answer is a pair of constants: /dev/random and /dev/urandom are
 * character device 1:8 and 1:9 and have been for as long as there has been a
 * Linux. On macOS the major belongs to whichever slot the random driver took in
 * the character device switch, which is a number about this boot and not about
 * the system, so there is nothing to write down. So the devices are asked for
 * by the names they are guaranteed to have, once, and what is compared after
 * that is st_rdev — the same question the constants were standing in for.
 *
 * The cache is two words written with the same two values by whoever gets there
 * first; `known` is set last, so a thread that reads it set reads the pair
 * complete. There is no lock because there is nothing a second writer could
 * make different.
 */
static dev_t	source_rdev[2];
static int	source_known;

static void
source_learn(void)
{
	struct stat st;
	dev_t rdev[2] = { 0, 0 };

	if (_sys_stat("/dev/random", &st) >= 0 && S_ISCHR(st.st_mode))
		rdev[0] = st.st_rdev;
	if (_sys_stat("/dev/urandom", &st) >= 0 && S_ISCHR(st.st_mode))
		rdev[1] = st.st_rdev;

	source_rdev[0] = rdev[0];
	source_rdev[1] = rdev[1];
	source_known = 1;
}

int
_entropy_is_source_fd(int fd)
{
	struct stat st;

	if (_syscall2(SYS_fstat64, fd, &st) < 0)
		return 0;
	if (!S_ISCHR(st.st_mode))
		return 0;

	if (!source_known)
		source_learn();

	return (source_rdev[0] && st.st_rdev == source_rdev[0]) ||
	       (source_rdev[1] && st.st_rdev == source_rdev[1]);
}
