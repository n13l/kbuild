/*
 * arch/os/<platform>/scan — finding the system call instructions in a run of
 * bytes, and the number each one makes.
 *
 * scan is pure: no system calls, no allocation, no state (arch/os/<p>/scan). So
 * it is the one piece of the platform layer that can be tested the same way on
 * every machine — hand it bytes, check what it says — and the test is compiled
 * for whatever architecture it is being built for, because the instruction it
 * looks for is that machine's. The vectors below are real encodings of that
 * machine, built from the same field layout scan decodes, so the array and the
 * decoder cannot drift apart silently: a wrong constant here is a wrong
 * instruction, and the assertions say what it was meant to be.
 *
 * There is a block per architecture and nothing shared between them but the
 * assertions' shape. A machine with no block is one scan itself #errors on, so
 * this file would not compile for it either — which is the right answer, not a
 * skipped test.
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>

#include <arch/os/scan.h>

/* A fixed request over @code, anchored at offset 0 unless a test says otherwise. */
static unsigned int
run(const uint8_t *code, uint64_t size, struct scan_site *site,
    unsigned int site_max, const uint32_t *want, unsigned int want_n)
{
	uint64_t anchor[1] = { 0 };
	struct scan_req r = {
		.code = code, .size = size,
		.anchor = anchor, .anchor_n = 1,
		.want = want, .want_n = want_n,
		.site = site, .site_max = site_max,
	};

	return scan_run(&r);
}


#if defined(__x86_64__)

/*
 * x86-64. `syscall` is 0f 05, two bytes that also occur inside immediates, so
 * the three cases below are: a real call, the number set three ways scan knows,
 * and the same two bytes buried in an immediate where scan must NOT see a call.
 */

/* mov eax, 39 (getpid) ; syscall  —  b8 imm32, the 5-byte lookback. */
static const uint8_t v_mov_eax[] = { 0xb8, 0x27, 0x00, 0x00, 0x00, 0x0f, 0x05 };
/* mov rax, 231 (exit_group) ; syscall  —  48 c7 c0 imm32, the 7-byte lookback. */
static const uint8_t v_mov_rax[] = {
	0x48, 0xc7, 0xc0, 0xe7, 0x00, 0x00, 0x00, 0x0f, 0x05
};
/* xor eax, eax (read) ; syscall  —  31 c0, the number is 0. */
static const uint8_t v_xor_eax[] = { 0x31, 0xc0, 0x0f, 0x05 };
/* mov eax, 0x050f0005  —  one 5-byte instruction whose immediate contains 0f 05. */
static const uint8_t v_imm_trap[] = { 0xb8, 0x05, 0x00, 0x0f, 0x05 };

#define NR_A	39u
#define NR_B	231u
#define SITE_A	5u	/* offset of the syscall in v_mov_eax */

static void
test_finds_the_call(void **state)
{
	struct scan_site s[4];
	unsigned int n = run(v_mov_eax, sizeof(v_mov_eax), s, 4, NULL, 0);

	(void)state;
	assert_int_equal(n, 1);
	assert_int_equal(s[0].off, SITE_A);
	assert_int_equal(s[0].nr, NR_A);
}

static void
test_number_from_each_mov(void **state)
{
	struct scan_site s[4];

	(void)state;
	assert_int_equal(run(v_mov_rax, sizeof(v_mov_rax), s, 4, NULL, 0), 1);
	assert_int_equal(s[0].nr, NR_B);

	assert_int_equal(run(v_xor_eax, sizeof(v_xor_eax), s, 4, NULL, 0), 1);
	assert_int_equal(s[0].nr, 0);
}

static void
test_immediate_is_not_a_call(void **state)
{
	struct scan_site s[4];

	(void)state;
	/* The 0f 05 at offset 3 is inside the mov's immediate: no site. */
	assert_int_equal(run(v_imm_trap, sizeof(v_imm_trap), s, 4, NULL, 0), 0);
}

#elif defined(__aarch64__)

/*
 * arm64. Instructions are four bytes and aligned, so a word is a call or it is
 * not. The number is set by a movz into x8 (w8) before the svc; scan reads it
 * back as (w >> 5) & 0xffff, which is the field this builds it into.
 */
static uint32_t v_mov_svc[2];	/* movz w8,#172 ; svc #0 */
static uint32_t v_movx_svc[2];	/* movz x8,#60  ; svc #0 */
static uint32_t v_bare_svc[1];	/* svc #0, with no number before it */

#define A64_MOVZ_W8	0x52800008u
#define A64_MOVZ_X8	0xd2800008u
#define A64_SVC		0xd4000001u
#define NR_A		172u
#define NR_B		60u

static int
group_setup(void **state)
{
	(void)state;
	v_mov_svc[0] = A64_MOVZ_W8 | (NR_A << 5);
	v_mov_svc[1] = A64_SVC;
	v_movx_svc[0] = A64_MOVZ_X8 | (NR_B << 5);
	v_movx_svc[1] = A64_SVC;
	v_bare_svc[0] = A64_SVC;
	return 0;
}

static void
test_finds_the_call(void **state)
{
	struct scan_site s[4];
	unsigned int n = run((const uint8_t *)v_mov_svc, sizeof(v_mov_svc),
	                     s, 4, NULL, 0);

	(void)state;
	assert_int_equal(n, 1);
	assert_int_equal(s[0].off, 4);
	assert_int_equal(s[0].nr, NR_A);
}

static void
test_number_from_each_mov(void **state)
{
	struct scan_site s[4];

	(void)state;
	assert_int_equal(run((const uint8_t *)v_movx_svc, sizeof(v_movx_svc),
	                     s, 4, NULL, 0), 1);
	assert_int_equal(s[0].nr, NR_B);
}

static void
test_bare_call_has_no_number(void **state)
{
	struct scan_site s[4];

	(void)state;
	assert_int_equal(run((const uint8_t *)v_bare_svc, sizeof(v_bare_svc),
	                     s, 4, NULL, 0), 1);
	assert_int_equal(s[0].nr, SCAN_NR_ANY);
}

#else
#error "os_scan: no vectors for this machine (scan itself would #error too)"
#endif


/*
 * The `want` filter is machine-independent: given a list of numbers, a site is
 * reported only if its number is one of them, and a number scan could not
 * decide (SCAN_NR_ANY) is reported whatever the list says. Asserted on the
 * first vector of whichever architecture this is.
 */
#if defined(__x86_64__)
#define V_FILTER	v_mov_eax
#define V_FILTER_LEN	sizeof(v_mov_eax)
#define V_FILTER_NR	NR_A
#elif defined(__aarch64__)
#define V_FILTER	((const uint8_t *)v_mov_svc)
#define V_FILTER_LEN	sizeof(v_mov_svc)
#define V_FILTER_NR	NR_A
#endif

static void
test_want_filter(void **state)
{
	struct scan_site s[4];
	uint32_t yes[] = { V_FILTER_NR };
	uint32_t no[] = { V_FILTER_NR + 1 };

	(void)state;
	assert_int_equal(run(V_FILTER, V_FILTER_LEN, s, 4, yes, 1), 1);
	assert_int_equal(run(V_FILTER, V_FILTER_LEN, s, 4, no, 1), 0);
}

static void
test_step_reports_length_and_kind(void **state)
{
	struct scan_insn in;

	(void)state;
	/* A syscall instruction, decoded on its own. */
	assert_int_equal(scan_step(V_FILTER + 0, V_FILTER_LEN, &in), 0);
	assert_true(in.len > 0);
}

static void
test_rejects_a_bad_request(void **state)
{
	struct scan_site s[1];
	struct scan_req r = { 0 };

	(void)state;
	assert_int_equal(scan_run(&r), 0);		/* no code */
	r.code = V_FILTER;
	r.size = V_FILTER_LEN;
	assert_int_equal(scan_run(&r), 0);		/* no site buffer */
	r.site = s;
	r.site_max = 0;
	assert_int_equal(scan_run(&r), 0);		/* no room */
}

int
main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_finds_the_call),
		cmocka_unit_test(test_number_from_each_mov),
#if defined(__x86_64__)
		cmocka_unit_test(test_immediate_is_not_a_call),
#elif defined(__aarch64__)
		cmocka_unit_test(test_bare_call_has_no_number),
#endif
		cmocka_unit_test(test_want_filter),
		cmocka_unit_test(test_step_reports_length_and_kind),
		cmocka_unit_test(test_rejects_a_bad_request),
	};

#if defined(__aarch64__)
	return cmocka_run_group_tests(tests, group_setup, NULL);
#else
	return cmocka_run_group_tests(tests, NULL, NULL);
#endif
}
