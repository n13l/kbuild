#include <arch/os/macos/scan/scan.h>

/*
 * The number register is x16, which is the one difference from the Linux step
 * beside the trap immediate: every stub in libsystem_kernel is written
 * `mov x16, #nr ; svc #0x80`, so the instruction before a site is what settles
 * what the site makes.
 */
#define A64_MOVZ_MASK	0xffe0001fu
#define A64_MOVZ_X16	0xd2800010u
#define A64_MOVZ_W16	0x52800010u

int
scan_step(const uint8_t *p, uint64_t left, struct scan_insn *in)
{
	uint32_t w;

	in->len = 0;
	in->kind = SCAN_OTHER;
	in->nr = SCAN_NR_ANY;

	if (left < SCAN_INSN_SIZE)
		return -1;

	__builtin_memcpy(&w, p, sizeof(w));
	in->len = SCAN_INSN_SIZE;

	if (w == SCAN_A64_SVC)
		in->kind = SCAN_SYSCALL;
	else if ((w & A64_MOVZ_MASK) == A64_MOVZ_X16 ||
	         (w & A64_MOVZ_MASK) == A64_MOVZ_W16) {
		in->kind = SCAN_SETNR;
		in->nr = (w >> 5) & 0xffffu;
	}

	return 0;
}

int
scan_lookback(const uint8_t *site, uint64_t back, uint32_t *nr)
{
	uint32_t w;

	if (back < SCAN_INSN_SIZE)
		return -1;

	__builtin_memcpy(&w, site - SCAN_INSN_SIZE, sizeof(w));
	if ((w & A64_MOVZ_MASK) != A64_MOVZ_X16 &&
	    (w & A64_MOVZ_MASK) != A64_MOVZ_W16)
		return -1;

	*nr = (w >> 5) & 0xffffu;
	return 0;
}
