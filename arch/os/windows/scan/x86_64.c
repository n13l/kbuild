#define scan_lookback	scan_lookback_adjacent
#include "../../linux/scan/x86_64.c"
#undef scan_lookback

int scan_lookback(const uint8_t *site, uint64_t back, uint32_t *nr);

static const uint8_t nt_stub_tail[] = {
	0xf6, 0x04, 0x25, 0x08, 0x03, 0xfe, 0x7f, 0x01,
	0x75, 0x03,
};

int
scan_lookback(const uint8_t *site, uint64_t back, uint32_t *nr)
{
	const uint64_t tail = sizeof(nt_stub_tail);
	unsigned int i;

	if (back >= tail + 5 && site[-(long)(tail + 5)] == 0xb8) {
		for (i = 0; i < tail; i++)
			if (site[-(long)tail + (long)i] != nt_stub_tail[i])
				break;
		if (i == tail) {
			__builtin_memcpy(nr, site - tail - 4, 4);
			return 0;
		}
	}

	return scan_lookback_adjacent(site, back, nr);
}
