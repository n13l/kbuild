#ifndef OS_MACOS_SCAN_H
#define OS_MACOS_SCAN_H

#include <stdint.h>
#include <stddef.h>

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


#if defined(__aarch64__)

#define SCAN_ARCH_NAME		"arm64"
#define SCAN_INSN_SIZE		4u
#define SCAN_INSN_ALIGN		4u

/*
 * `svc #0x80`, not `svc #0`. Darwin puts the trap number in the immediate and
 * the system call number in x16, where Linux traps on #0 and reads x8 — so the
 * word this looks for and the word arch/os/linux/scan looks for are different
 * words, and a scan built for the wrong one finds nothing rather than finding
 * the wrong thing.
 *
 * Armed is `udf #0`, the same all-zero word: an encoding that is permanently
 * undefined on this machine as on the other, and one aligned store away.
 */
#define SCAN_A64_SVC		0xd4001001u
#define SCAN_A64_UDF		0x00000000u

static inline int
scan_is_call(const void *at)
{
	return *(const volatile uint32_t *)at == SCAN_A64_SVC;
}

static inline int
scan_is_armed(const void *at)
{
	return *(const volatile uint32_t *)at == SCAN_A64_UDF;
}

static inline void
scan_arm(void *at)
{
	*(volatile uint32_t *)at = SCAN_A64_UDF;
}

static inline void
scan_disarm(void *at)
{
	*(volatile uint32_t *)at = SCAN_A64_SVC;
}

#else
#error "arch/os/macos/scan: no syscall instruction is known for this machine"
#endif

static inline void
scan_sync(void *at, size_t len)
{
	__builtin___clear_cache((char *)at, (char *)at + len);
}

#endif
