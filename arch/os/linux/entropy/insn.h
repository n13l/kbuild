
#ifndef OS_LINUX_ENTROPY_INSN_H
#define OS_LINUX_ENTROPY_INSN_H

#include <stdint.h>

enum entropy_insn {
	ENTROPY_INSN_NONE = 0,
	ENTROPY_INSN_JITTER_TIMER,
	ENTROPY_INSN_HW_RNG,
};

#if defined(__aarch64__)

#define ENTROPY_A64_CNTVCT_EL0		0xd53be040u
#define ENTROPY_A64_CNTVCTSS_EL0	0xd53be0c0u
#define ENTROPY_A64_CNTPCT_EL0		0xd53be020u
#define ENTROPY_A64_RNDR		0xd53b2400u
#define ENTROPY_A64_RNDRRS		0xd53b2420u

static inline enum entropy_insn
entropy_insn_kind(uint32_t insn, int *rt)
{
	uint32_t base = insn & 0xffffffe0u;

	if (rt)
		*rt = (int)(insn & 0x1fu);

	switch (base) {
	case ENTROPY_A64_CNTVCT_EL0:
	case ENTROPY_A64_CNTVCTSS_EL0:
	case ENTROPY_A64_CNTPCT_EL0:
		return ENTROPY_INSN_JITTER_TIMER;
	case ENTROPY_A64_RNDR:
	case ENTROPY_A64_RNDRRS:
		return ENTROPY_INSN_HW_RNG;
	default:
		return ENTROPY_INSN_NONE;
	}
}

#else

static inline enum entropy_insn
entropy_insn_kind(uint32_t insn, int *rt)
{
	(void)insn;
	if (rt)
		*rt = -1;
	return ENTROPY_INSN_NONE;
}

#endif

static inline const char *
entropy_insn_name(enum entropy_insn k)
{
	switch (k) {
	case ENTROPY_INSN_JITTER_TIMER:	return "jitter-timer";
	case ENTROPY_INSN_HW_RNG:	return "hw-rng";
	default:			return "none";
	}
}

#endif
