/*
 * arch/os/<platform>/entropy — the one call a program draws randomness with,
 * made the freestanding way io is (getrandom's shape over whatever the kernel
 * has: getrandom on Linux, getentropy on macOS, BCryptGenRandom on Windows).
 *
 * The real call, run natively: ask for bytes, check the count, and that a
 * second draw is not the first. Randomness cannot be asserted — any fixed
 * bytes are a legal result — so what is pinned is the contract: the length
 * comes back, the buffer around it is untouched, and two large draws differ
 * (two 256-byte buffers colliding by chance is 2^-2048, which is never).
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include <arch/os/entropy.h>

static void
test_fills_and_returns_the_count(void **state)
{
	unsigned char buf[64];
	long n;

	(void)state;
	memset(buf, 0, sizeof(buf));
	n = _sys_getrandom(buf, sizeof(buf), 0);
	assert_int_equal(n, (long)sizeof(buf));
}

static void
test_respects_the_bounds(void **state)
{
	unsigned char buf[16];
	long n;

	(void)state;
	/* A sentinel on each side: the fill must stay within [1, len-1]. */
	buf[0] = 0xAA;
	buf[15] = 0xBB;
	n = _sys_getrandom(buf + 1, 14, 0);
	assert_int_equal(n, 14);
	assert_int_equal(buf[0], 0xAA);
	assert_int_equal(buf[15], 0xBB);
}

static void
test_zero_length(void **state)
{
	unsigned char b = 0x7F;

	(void)state;
	/* Nothing asked for, nothing drawn, and the byte is left alone. */
	assert_int_equal(_sys_getrandom(&b, 0, 0), 0);
	assert_int_equal(b, 0x7F);
}

static void
test_two_draws_differ(void **state)
{
	unsigned char a[256], b[256];

	(void)state;
	assert_int_equal(_sys_getrandom(a, sizeof(a), 0), (long)sizeof(a));
	assert_int_equal(_sys_getrandom(b, sizeof(b), 0), (long)sizeof(b));
	assert_memory_not_equal(a, b, sizeof(a));
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_fills_and_returns_the_count),
		cmocka_unit_test(test_respects_the_bounds),
		cmocka_unit_test(test_zero_length),
		cmocka_unit_test(test_two_draws_differ),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
