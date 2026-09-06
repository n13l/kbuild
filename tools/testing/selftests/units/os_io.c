/*
 * arch/os/<platform>/io — the system call, the handful a program makes when it
 * has no C library under it to make them through.
 *
 * These are the real calls, not a mock: the test opens a file, writes it, reads
 * it back, stats it, and so on, through _sys_* and checks the kernel did what
 * was asked. So it runs where it can make the call — natively on the build host
 * — and what it is pinned to is the contract every caller in this tree is
 * written against: the Linux convention, a negative return being -errno,
 * whatever the platform underneath spells its errors in (macOS folds a carry
 * flag, Windows translates GetLastError, and this file cannot tell which).
 *
 * The scratch file is made under the directory the harness gives the test
 * (TMPDIR), removed at the end, and named with the pid so two runs do not
 * collide.
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <cmocka.h>

#include <arch/os/io.h>

static char path[256];
static char link_path[256];

static int
group_setup(void **state)
{
	const char *dir = getenv("TMPDIR");
	long pid = _sys_getpid();

	(void)state;
	if (!dir || !*dir)
		dir = "/tmp";
	snprintf(path, sizeof(path), "%s/os_io.%ld", dir, pid);
	snprintf(link_path, sizeof(link_path), "%s/os_io_link.%ld", dir, pid);
	unlink(path);
	unlink(link_path);
	return 0;
}

static int
group_teardown(void **state)
{
	(void)state;
	unlink(path);
	unlink(link_path);
	return 0;
}

/* open -> write -> close, then open -> read -> close, and the bytes match. */
static void
test_write_then_read(void **state)
{
	static const char msg[] = "the quick brown fox";
	char buf[64];
	long fd, n;

	(void)state;
	fd = _sys_open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	assert_true(fd >= 0);
	n = _sys_write((int)fd, msg, sizeof(msg) - 1);
	assert_int_equal(n, (long)(sizeof(msg) - 1));
	assert_int_equal(_sys_close((int)fd), 0);

	fd = _sys_open(path, O_RDONLY, 0);
	assert_true(fd >= 0);
	n = _sys_read((int)fd, buf, sizeof(buf));
	assert_int_equal(n, (long)(sizeof(msg) - 1));
	buf[n] = '\0';
	assert_string_equal(buf, msg);
	assert_int_equal(_sys_close((int)fd), 0);
}

/* A negative return is -errno, and opening what is not there is -ENOENT. */
static void
test_missing_file_is_enoent(void **state)
{
	long fd = _sys_open("/no/such/path/os_io", O_RDONLY, 0);

	(void)state;
	assert_true(fd < 0);
	assert_int_equal((int)-fd, ENOENT);
}

/* stat sees the size that was written, and the mode says a regular file. */
static void
test_stat(void **state)
{
	struct stat st;

	(void)state;
	memset(&st, 0, sizeof(st));
	assert_int_equal(_sys_stat(path, &st), 0);
	assert_int_equal((long)st.st_size, 19);
	assert_true(S_ISREG(st.st_mode));
}

/* access answers the question the file's permissions settle. */
static void
test_access(void **state)
{
	(void)state;
	assert_int_equal(_sys_access(path, IO_F_OK), 0);
	assert_int_equal(_sys_access(path, IO_R_OK), 0);
	assert_true(_sys_access("/no/such/path/os_io", IO_F_OK) < 0);
}

/* readlink gives back the target a symlink was made with. */
static void
test_readlink(void **state)
{
	char buf[256];
	long n;

	(void)state;
	unlink(link_path);
	if (symlink(path, link_path) != 0)
		skip();		/* no symlink support here (e.g. some mounts) */

	n = _sys_readlink(link_path, buf, sizeof(buf));
	assert_true(n > 0);
	buf[n] = '\0';
	assert_string_equal(buf, path);
}

/* getpid is this process, getppid is not it, and both are positive. */
static void
test_pid(void **state)
{
	long pid = _sys_getpid();
	long ppid = _sys_getppid();

	(void)state;
	assert_true(pid > 0);
	assert_true(ppid > 0);
	assert_int_not_equal(pid, ppid);
	assert_int_equal(pid, (long)getpid());
}

/* gettid is a thread id, and in a single-threaded program it is the pid. */
static void
test_gettid(void **state)
{
	long tid = _sys_gettid();

	(void)state;
	assert_true(tid > 0);
	assert_int_equal(tid, _sys_getpid());
}

/* gettimeofday moves forward and reports a plausible wall clock (after 2020). */
static void
test_gettimeofday(void **state)
{
	struct timeval tv;

	(void)state;
	memset(&tv, 0, sizeof(tv));
	assert_int_equal(_sys_gettimeofday(&tv), 0);
	assert_true(tv.tv_sec > 1577836800L);		/* 2020-01-01 */
	assert_true(tv.tv_usec >= 0 && tv.tv_usec < 1000000);
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_write_then_read),
		cmocka_unit_test(test_missing_file_is_enoent),
		cmocka_unit_test(test_stat),
		cmocka_unit_test(test_access),
		cmocka_unit_test(test_readlink),
		cmocka_unit_test(test_pid),
		cmocka_unit_test(test_gettid),
		cmocka_unit_test(test_gettimeofday),
	};

	return cmocka_run_group_tests(tests, group_setup, group_teardown);
}
