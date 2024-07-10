
#ifndef OS_LINUX_SCAN_H
#define OS_LINUX_SCAN_H

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


#if defined(__x86_64__)

#define SCAN_ARCH_NAME		"x86-64"
#define SCAN_INSN_SIZE		2u
#define SCAN_INSN_ALIGN		1u

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

#define SCAN_A64_SVC		0xd4000001u
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
#error "arch/os/linux/scan: no syscall instruction is known for this machine"
#endif

static inline void
scan_sync(void *at, size_t len)
{
	__builtin___clear_cache((char *)at, (char *)at + len);
}

#endif
