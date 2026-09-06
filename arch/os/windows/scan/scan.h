#ifndef OS_WINDOWS_SCAN_H
#define OS_WINDOWS_SCAN_H

#include <stdint.h>
#include <stddef.h>

/*
 * The same interface as arch/os/linux/scan and arch/os/macos/scan, field for
 * field — x86_64.c here compiles the Linux decoder against the Linux header,
 * and the two must agree on every structure below for that to link.
 *
 * What a site is on Windows is ntdll's business and nobody else's: the Nt and
 * Zw stubs are the only code that makes system calls, the number each one makes is
 * fixed by the build of Windows it ships in, and renumbered by the next one. So
 * the number a site reports is a fact about this ntdll, not a name, and what
 * makes it useful is the stub it was found in.
 */
#define SCAN_NR_ANY		0xffffffffu

struct scan_site {
	uint64_t	off;
	uint32_t	nr;
};

struct scan_req {
	const uint8_t		*code;
	uint64_t		size;
	const uint64_t		*anchor;
	unsigned int		anchor_n;
	const uint32_t		*want;
	unsigned int		want_n;
	struct scan_site	*site;
	unsigned int		site_max;
};

unsigned int scan_run(const struct scan_req *r);

#define SCAN_OTHER		0
#define SCAN_SYSCALL		1
#define SCAN_SETNR		2

struct scan_insn {
	unsigned int	len;
	unsigned int	kind;
	uint32_t	nr;
};

int scan_step(const uint8_t *p, uint64_t left, struct scan_insn *in);

int scan_lookback(const uint8_t *site, uint64_t back, uint32_t *nr);


#if defined(__x86_64__)

#define SCAN_ARCH_NAME		"x86-64"
#define SCAN_INSN_SIZE		2u
#define SCAN_INSN_ALIGN		1u

/*
 * `syscall`, the same two bytes as on Linux and armed the same way, as `ud2`.
 * Every stub also carries an `int 2e` a few bytes on — the older way in — and
 * that one is not reported: `cd 2e` has no one-byte store that makes it
 * undefined and back, and the stub only reaches it by a branch.
 */
static inline int
scan_is_call(const void *at)
{
	const uint8_t *p = (const uint8_t *)at;

	return p[0] == 0x0f && p[1] == 0x05;
}

static inline int
scan_is_armed(const void *at)
{
	const uint8_t *p = (const uint8_t *)at;

	return p[0] == 0x0f && p[1] == 0x0b;
}

static inline void
scan_arm(void *at)
{
	((volatile uint8_t *)at)[1] = 0x0b;
}

static inline void
scan_disarm(void *at)
{
	((volatile uint8_t *)at)[1] = 0x05;
}

#elif defined(__aarch64__)

#define SCAN_ARCH_NAME		"arm64"
#define SCAN_INSN_SIZE		4u
#define SCAN_INSN_ALIGN		4u

/*
 * `svc #N` with the call number in the immediate. Linux traps on #0 and reads
 * x8, macOS on #0x80 and reads x16; Windows encodes the call in the instruction
 * itself, so a site is any svc and its number is read off the word. Armed is
 * `udf #N` — the same sixteen bits in an encoding that is permanently undefined
 * — so the trap carries the number, and disarming is possible at all (there is
 * no constant to restore, the way Linux and macOS have one).
 */
#define SCAN_A64_SVC_MASK	0xffe0001fu
#define SCAN_A64_SVC		0xd4000001u
#define SCAN_A64_UDF_MASK	0xffff0000u
#define SCAN_A64_UDF		0x00000000u

static inline uint32_t
scan_a64_svc_nr(uint32_t w)
{
	return (w >> 5) & 0xffffu;
}

static inline int
scan_is_call(const void *at)
{
	return (*(const volatile uint32_t *)at & SCAN_A64_SVC_MASK) ==
	       SCAN_A64_SVC;
}

static inline int
scan_is_armed(const void *at)
{
	return (*(const volatile uint32_t *)at & SCAN_A64_UDF_MASK) ==
	       SCAN_A64_UDF;
}

static inline void
scan_arm(void *at)
{
	uint32_t w = *(volatile uint32_t *)at;

	*(volatile uint32_t *)at = SCAN_A64_UDF | scan_a64_svc_nr(w);
}

static inline void
scan_disarm(void *at)
{
	uint32_t w = *(volatile uint32_t *)at;

	*(volatile uint32_t *)at = SCAN_A64_SVC | ((w & 0xffffu) << 5);
}

#else
#error "arch/os/windows/scan: no syscall instruction is known for this machine"
#endif

static inline void
scan_sync(void *at, size_t len)
{
	__builtin___clear_cache((char *)at, (char *)at + len);
}

#endif
