/*
 * arch/os/str.h — the freestanding C-string helpers a program linked with no
 * libc still needs, and the one rule about reaching them.
 *
 * This header is the same file on every platform (there is no kernel in a byte
 * loop over a NUL-terminated string), so it is tested once, here, against the
 * nolibc_ names directly — the implementations, not the xstr* dispatch, which
 * only chooses between these and the C library's and is a build-time switch
 * rather than behaviour. strlcpy/strlcat are this tree's (dst, size, src)
 * order, not the BSD one, so the order is what the assertions pin.
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include <arch/os/str.h>

static void
test_strlen(void **state)
{
	(void)state;
	assert_int_equal(nolibc_strlen(""), 0);
	assert_int_equal(nolibc_strlen("abc"), 3);
	assert_int_equal(nolibc_strlen("a\0c"), 1);
}

static void
test_strcmp(void **state)
{
	(void)state;
	assert_int_equal(nolibc_strcmp("abc", "abc"), 0);
	assert_true(nolibc_strcmp("abc", "abd") < 0);
	assert_true(nolibc_strcmp("abd", "abc") > 0);
	assert_true(nolibc_strcmp("ab", "abc") < 0);
	/* Bytes compare unsigned: 0x80 is greater than 'a', not less. */
	assert_true(nolibc_strcmp("\x80", "a") > 0);
}

static void
test_strncmp(void **state)
{
	(void)state;
	assert_int_equal(nolibc_strncmp("abc", "abd", 2), 0);
	assert_true(nolibc_strncmp("abc", "abd", 3) < 0);
	assert_int_equal(nolibc_strncmp("abc", "abc", 100), 0);
	assert_int_equal(nolibc_strncmp("x", "y", 0), 0);
}

static void
test_strrchr(void **state)
{
	const char *s = "a/b/c";

	(void)state;
	assert_ptr_equal(nolibc_strrchr(s, '/'), s + 3);
	assert_ptr_equal(nolibc_strrchr(s, 'a'), s);
	assert_null(nolibc_strrchr(s, 'z'));
	/*
	 * This copy stops at the terminator and does not match it, so a search
	 * for '\0' is NULL here where libc's strrchr returns the end. xstrrchr
	 * resolves to one or the other by build, and no caller in this tree
	 * searches for the NUL; the divergence is pinned so it stays known.
	 */
	assert_null(nolibc_strrchr(s, '\0'));
}

static void
test_strstr(void **state)
{
	const char *s = "hello world";

	(void)state;
	assert_ptr_equal(nolibc_strstr(s, "world"), s + 6);
	assert_ptr_equal(nolibc_strstr(s, ""), s);	/* empty needle: the start */
	assert_null(nolibc_strstr(s, "xyz"));
	assert_ptr_equal(nolibc_strstr(s, "hello"), s);
}

static void
test_strlcpy(void **state)
{
	char buf[8];
	size_t n;

	(void)state;
	/* Returns the source length, and always NUL-terminates. */
	n = nolibc_strlcpy(buf, sizeof(buf), "abc");
	assert_int_equal(n, 3);
	assert_string_equal(buf, "abc");

	/* Truncates to size-1 and still terminates; the return is the source. */
	n = nolibc_strlcpy(buf, sizeof(buf), "0123456789");
	assert_int_equal(n, 7);
	assert_string_equal(buf, "0123456");

	/* A zero size writes nothing and returns 0. */
	buf[0] = 'Z';
	assert_int_equal(nolibc_strlcpy(buf, 0, "x"), 0);
	assert_int_equal(buf[0], 'Z');
}

static void
test_strlcat(void **state)
{
	char buf[8];

	(void)state;
	nolibc_strlcpy(buf, sizeof(buf), "ab");
	assert_int_equal(nolibc_strlcat(buf, sizeof(buf), "cd"), 4);
	assert_string_equal(buf, "abcd");

	/* Appending past the end truncates and terminates. */
	nolibc_strlcpy(buf, sizeof(buf), "abcde");
	nolibc_strlcat(buf, sizeof(buf), "XYZ");
	assert_string_equal(buf, "abcdeXY");
}

static void
test_utoa(void **state)
{
	char buf[24];

	(void)state;
	assert_string_equal(nolibc_utoa(0, buf, sizeof(buf)), "0");
	assert_string_equal(nolibc_utoa(1, buf, sizeof(buf)), "1");
	assert_string_equal(nolibc_utoa(4294967295UL, buf, sizeof(buf)),
	                    "4294967295");
	assert_string_equal(nolibc_utoa(1000000000000ULL, buf, sizeof(buf)),
	                    "1000000000000");
	/* Too small a buffer gives back an empty string, not an overrun. */
	assert_string_equal(nolibc_utoa(123, buf, 1), "");
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_strlen),
		cmocka_unit_test(test_strcmp),
		cmocka_unit_test(test_strncmp),
		cmocka_unit_test(test_strrchr),
		cmocka_unit_test(test_strstr),
		cmocka_unit_test(test_strlcpy),
		cmocka_unit_test(test_strlcat),
		cmocka_unit_test(test_utoa),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
