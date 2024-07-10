
#include <arch/os/linux/scan/scan.h>

#define A64_MOVZ_MASK	0xffe0001fu
#define A64_MOVZ_X8	0xd2800008u
#define A64_MOVZ_W8	0x52800008u

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
	else if ((w & A64_MOVZ_MASK) == A64_MOVZ_X8 ||
	         (w & A64_MOVZ_MASK) == A64_MOVZ_W8) {
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
	if ((w & A64_MOVZ_MASK) != A64_MOVZ_X8 &&
	    (w & A64_MOVZ_MASK) != A64_MOVZ_W8)
		return -1;

	*nr = (w >> 5) & 0xffffu;
	return 0;
}
