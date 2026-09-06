
#include <arch/os/windows/scan/scan.h>

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

	if ((w & SCAN_A64_SVC_MASK) == SCAN_A64_SVC) {
		in->kind = SCAN_SYSCALL;
		in->nr = scan_a64_svc_nr(w);
	}

	return 0;
}

int
scan_lookback(const uint8_t *site, uint64_t back, uint32_t *nr)
{
	uint32_t w;

	(void)back;

	__builtin_memcpy(&w, site, sizeof(w));
	if ((w & SCAN_A64_SVC_MASK) != SCAN_A64_SVC)
		return -1;

	*nr = scan_a64_svc_nr(w);
	return 0;
}
