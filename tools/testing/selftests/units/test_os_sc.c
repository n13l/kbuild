/*
 * arch/os/linux/sc — arming this process's own system call instructions and
 * standing behind the trap they raise. The in-process half, Linux only.
 *
 * This is the one piece of the platform layer that cannot be a pure-function
 * unit: it writes over text, takes the SIGILL that text now raises, and makes
 * the call from the handler. So the test is the mechanism end to end, in one
 * process: watch getpid, arm self, make a getpid, and assert the trap was
 * dispatched — the callback saw the number and the real return — while the call
 * still returned the right pid to the caller.
 *
 * Two things it pins that are easy to get wrong:
 *
 *   The watched call is made through the C library, not through this tree's own
 *   <arch/os/io.h>. sc never arms the image sc itself is in (sc_walk_self skips
 *   it, so the dispatcher and the io it makes its own calls with are never among
 *   what is armed), and in this one binary sc and io are the same image. That is
 *   exactly how ub works too — sc lives in the agent .so and arms the program
 *   around it — so trapping a libc call here is the faithful case, and the same
 *   skip is asserted below: a call through io does NOT trap.
 *
 *   Standalone rather than cmocka: cmocka has its own signal handling and runs
 *   cases behind setjmp, and a test that installs a SIGILL dispatcher and
 *   rewrites live code wants neither between it and the kernel. It prints TAP so
 *   the bats case that runs it reads one line per check, and exits non-zero if
 *   any failed.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <arch/os/io.h>
#include <arch/os/sc.h>

static volatile int	seen;
static volatile long	seen_nr;
static volatile long	seen_ret;

static void
on_call(const struct sc_call *c)
{
	seen++;
	seen_nr = c->nr;
	seen_ret = c->ret;
}

static int	done, failed;

static void
ok(int cond, const char *name)
{
	done++;
	if (cond) {
		printf("ok %d - %s\n", done, name);
	} else {
		printf("not ok %d - %s\n", done, name);
		failed++;
	}
}

/* getpid through the C library — a different image from sc, so it is armed. */
static long
libc_getpid(void)
{
	return syscall(SYS_getpid);
}

int
main(void)
{
	static const uint32_t watch[] = { (uint32_t)SYS_getpid };
	struct sc_cfg cfg;
	long before, after, own;
	int armed, seen_at_own;

	printf("1..6\n");

	before = libc_getpid();
	ok(before > 0, "getpid before arming returns a pid");

	memset(&cfg, 0, sizeof(cfg));
	cfg.watch = watch;
	cfg.watch_n = 1;
	cfg.report = on_call;

	ok(sc_open(&cfg) == 0, "sc_open");

	armed = sc_arm_self();
	ok(armed > 0, "sc_arm_self armed at least one site");

	/*
	 * The same call again. Its instruction in the C library is now
	 * undefined; executing it traps, the dispatcher performs getpid and
	 * returns it here, and the callback records what went by.
	 */
	seen = 0;
	after = libc_getpid();

	ok(after == before, "the trapped getpid still returned the right pid");
	ok(seen >= 1 && seen_nr == SYS_getpid && seen_ret == before,
	   "the dispatcher reported the call, its number and its return");

	/*
	 * The same call through this tree's own io, which is in sc's own image.
	 * sc does not arm its own image, so this must NOT trap: the callback
	 * count does not move, and the call still returns the pid.
	 */
	seen_at_own = seen;
	own = _sys_getpid();
	ok(own == before && seen == seen_at_own,
	   "a call in sc's own image is not armed");

	sc_close();

	if (failed)
		printf("# %d of %d checks failed\n", failed, done);
	return failed ? 1 : 0;
}
