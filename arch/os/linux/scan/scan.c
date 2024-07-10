
#include <arch/os/linux/scan/scan.h>

static int
wanted(const struct scan_req *r, uint32_t nr)
{
	unsigned int i;

	if (!r->want || !r->want_n || nr == SCAN_NR_ANY)
		return 1;

	for (i = 0; i < r->want_n; i++)
		if (r->want[i] == nr)
			return 1;
	return 0;
}

static unsigned int
sweep(const struct scan_req *r, uint64_t from, uint64_t to, unsigned int found)
{
	uint32_t nr = SCAN_NR_ANY;
	uint64_t at = from;
	int have = 0;

	while (at < to && found < r->site_max) {
		struct scan_insn in;

		if (scan_step(r->code + at, r->size - at, &in) || !in.len)
			break;

		if (in.kind == SCAN_SYSCALL) {
			uint32_t n = have ? nr : SCAN_NR_ANY;

			if (n == SCAN_NR_ANY)
				scan_lookback(r->code + at, at, &n);

			if (wanted(r, n)) {
				r->site[found].off = at;
				r->site[found].nr = n;
				found++;
			}
		}

		have = in.kind == SCAN_SETNR;
		nr = have ? in.nr : SCAN_NR_ANY;
		at += in.len;
	}
	return found;
}

unsigned int
scan_run(const struct scan_req *r)
{
	unsigned int found = 0, i;

	if (!r || !r->code || !r->site || !r->site_max || r->size < SCAN_INSN_SIZE)
		return 0;

	if (!r->anchor || !r->anchor_n)
		return sweep(r, 0, r->size, 0);

	for (i = 0; i < r->anchor_n && found < r->site_max; i++) {
		uint64_t from = r->anchor[i];
		uint64_t to = i + 1 < r->anchor_n ? r->anchor[i + 1] : r->size;

		if (from >= r->size)
			continue;
		if (to > r->size)
			to = r->size;
		if (to <= from)
			continue;

		found = sweep(r, from, to, found);
	}
	return found;
}
