
#include <arch/os/linux/scan/scan.h>

#define F_M	0x01u
#define F_I1	0x02u
#define F_I2	0x04u
#define F_IZ	0x08u
#define F_IV	0x10u
#define F_IA	0x20u
#define F_G3	0x40u
#define F_BAD	0x80u
#define F_IR	0x100u

#define F_IMM	(F_I1 | F_I2 | F_IZ | F_IV | F_IA | F_IR)

static const uint16_t map1[256] = {
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, F_BAD, F_BAD,
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, F_BAD, 0,
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, F_BAD, F_BAD,
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, F_BAD, F_BAD,
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, 0, F_BAD,
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, 0, F_BAD,
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, 0, F_BAD,
	  F_M, F_M, F_M, F_M, F_I1, F_IZ, 0, F_BAD,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  F_BAD, F_BAD, 0, F_M, 0, 0, 0, 0,
	  F_IZ, F_M | F_IZ, F_I1, F_M | F_I1, 0, 0, 0, 0,
	  F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1,
	  F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1,
	  F_M | F_I1, F_M | F_IZ, F_BAD, F_M | F_I1, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, F_BAD, 0, 0, 0, 0, 0,
	  F_IA, F_IA, F_IA, F_IA, 0, 0, 0, 0,
	  F_I1, F_IZ, 0, 0, 0, 0, 0, 0,
	  F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1,
	  F_IV, F_IV, F_IV, F_IV, F_IV, F_IV, F_IV, F_IV,
	  F_M | F_I1, F_M | F_I1, F_I2, 0, 0, 0, F_M | F_I1, F_M | F_IZ,
	  F_I2 | F_I1, 0, F_I2, 0, 0, F_I1, F_BAD, 0,
	  F_M, F_M, F_M, F_M, F_BAD, F_BAD, F_BAD, 0,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1, F_I1,
	  F_IR, F_IR, F_BAD, F_I1, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0,
	  F_M | F_G3 | F_I1, F_M | F_G3 | F_IZ,
	  0, 0, 0, 0, 0, 0, F_M, F_M,
};

static const uint16_t map2[256] = {
	  F_M, F_M, F_M, F_M, F_BAD, 0, 0, 0,
	  0, 0, F_BAD, 0, F_BAD, F_M, 0, F_M | F_I1,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_BAD, F_BAD, F_BAD, F_BAD,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  0, 0, 0, 0, 0, 0, F_BAD, 0,
	  0, F_BAD, 0, F_BAD, F_BAD, F_BAD, F_BAD, F_BAD,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M | F_I1, F_M | F_I1, F_M | F_I1, F_M | F_I1, F_M, F_M, F_M, 0,
	  F_M, F_M, F_BAD, F_BAD, F_M, F_M, F_M, F_M,
	  F_IR, F_IR, F_IR, F_IR, F_IR, F_IR, F_IR, F_IR,
	  F_IR, F_IR, F_IR, F_IR, F_IR, F_IR, F_IR, F_IR,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  0, 0, 0, F_M, F_M | F_I1, F_M, F_BAD, F_BAD,
	  0, 0, 0, F_M, F_M | F_I1, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M | F_I1, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M | F_I1, F_M, F_M | F_I1, F_M | F_I1, F_M | F_I1, F_M,
	  0, 0, 0, 0, 0, 0, 0, 0,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
	  F_M, F_M, F_M, F_M, F_M, F_M, F_M, F_M,
};

#define OP_ESC0F	0x0fu
#define OP_VEX3		0xc4u
#define OP_VEX2		0xc5u
#define OP_EVEX		0x62u
#define OP_XOP		0x8fu

#define MAP_1		1u
#define MAP_0F		2u
#define MAP_0F38	3u
#define MAP_0F3A	4u
#define MAP_XOP8	5u
#define MAP_XOP9	6u
#define MAP_XOPA	7u

static int
is_legacy_prefix(uint8_t b)
{
	return b == 0x26 || b == 0x2e || b == 0x36 || b == 0x3e ||
	       b == 0x64 || b == 0x65 || b == 0xf0 || b == 0xf2 || b == 0xf3;
}

static int
sets_eax(const uint8_t *op, unsigned int len, unsigned int map, uint8_t rex,
         int osz66, uint32_t *nr)
{
	if (map != MAP_1 || osz66 || (rex & 0x05u))
		return 0;

	if (op[0] == 0xb8 && len >= 5 && !(rex & 0x08u)) {
		uint32_t v;

		__builtin_memcpy(&v, op + 1, 4);
		*nr = v;
		return 1;
	}

	if (op[0] == 0xc7 && len >= 6 && op[1] == 0xc0) {
		uint32_t v;

		__builtin_memcpy(&v, op + 2, 4);
		*nr = v;
		return 1;
	}

	if ((op[0] == 0x31 || op[0] == 0x33) && len >= 2 && op[1] == 0xc0) {
		*nr = 0;
		return 1;
	}

	return 0;
}

int
scan_step(const uint8_t *p, uint64_t left, struct scan_insn *in)
{
	const uint8_t *op;
	unsigned int map = MAP_1, imm = 0, n = 0;
	unsigned int flags;
	uint8_t rex = 0, b;
	int osz66 = 0, asz67 = 0, vex = 0;

	in->len = 0;
	in->kind = SCAN_OTHER;
	in->nr = SCAN_NR_ANY;

	for (;;) {
		if (n >= left || n >= 15)
			return -1;
		b = p[n];
		if (is_legacy_prefix(b))
			n++;
		else if (b == 0x66)
			osz66 = 1, n++;
		else if (b == 0x67)
			asz67 = 1, n++;
		else if (b >= 0x40 && b <= 0x4f)
			rex = b, n++;
		else
			break;
	}

	op = p + n;
	if (n >= left)
		return -1;

	if (b == OP_ESC0F) {
		if (n + 1 >= left)
			return -1;
		n++;
		if (p[n] == 0x38 || p[n] == 0x3a) {
			map = p[n] == 0x38 ? MAP_0F38 : MAP_0F3A;
			n++;
		} else {
			map = MAP_0F;
		}
	} else if (b == OP_VEX2) {
		if (n + 1 >= left)
			return -1;
		n += 2;
		map = MAP_0F;
		vex = 1;
	} else if (b == OP_XOP && n + 1 < left && (p[n + 1] & 0x38u)) {
		unsigned int mm = p[n + 1] & 0x1fu;

		if (n + 3 >= left)
			return -1;
		if (p[n + 2] & 0x80u)
			rex |= 0x08u;
		n += 3;
		if (mm == 8)
			map = MAP_XOP8;
		else if (mm == 9)
			map = MAP_XOP9;
		else if (mm == 10)
			map = MAP_XOPA;
		else
			return -1;
		vex = 1;
	} else if (b == OP_VEX3 || b == OP_EVEX) {
		unsigned int pay = b == OP_VEX3 ? 2 : 3;
		unsigned int mm;

		if (n + pay >= left)
			return -1;
		mm = p[n + 1] & (b == OP_VEX3 ? 0x1fu : 0x07u);

		if (p[n + 2] & 0x80u)
			rex |= 0x08u;
		n += 1 + pay;
		if (mm == 1)
			map = MAP_0F;
		else if (mm == 2)
			map = MAP_0F38;
		else if (mm == 3)
			map = MAP_0F3A;
		else
			return -1;
		vex = 1;
	}

	if (n >= left)
		return -1;
	op = p + n;
	b = p[n++];

	if (map == MAP_1)
		flags = map1[b];
	else if (map == MAP_0F)
		flags = map2[b];
	else if (map == MAP_0F38 || map == MAP_XOP9)
		flags = F_M;
	else if (map == MAP_XOPA)
		flags = F_M | F_IZ;
	else
		flags = F_M | F_I1;

	if (vex) {
		if (!(map == MAP_0F && b == 0x77))
			flags |= F_M;
		if (map != MAP_XOPA)
			flags &= ~(unsigned int)(F_IZ | F_IV | F_IA | F_G3 |
			                         F_BAD);
		else
			flags &= ~(unsigned int)(F_IV | F_IA | F_G3 | F_BAD);
	}

	if (flags & F_BAD)
		return -1;

	if (flags & F_M) {
		uint8_t modrm, mod, rm, reg;

		if (n >= left)
			return -1;
		modrm = p[n++];
		mod = (uint8_t)(modrm >> 6);
		reg = (uint8_t)((modrm >> 3) & 7);
		rm = (uint8_t)(modrm & 7);

		if (mod != 3) {
			unsigned int disp = 0;
			uint8_t base = 0;

			if (rm == 4) {
				if (n >= left)
					return -1;
				base = (uint8_t)(p[n++] & 7);
			}
			if (mod == 0) {
				if (rm == 5 || (rm == 4 && base == 5))
					disp = 4;
			} else {
				disp = mod == 1 ? 1u : 4u;
			}
			n += disp;
		}

		if ((flags & F_G3) && reg > 1)
			flags &= ~(unsigned int)F_IMM;
	}

	if (flags & F_I1)
		imm += 1;
	if (flags & F_I2)
		imm += 2;
	if (flags & F_IZ)
		imm += osz66 ? 2u : 4u;
	if (flags & F_IV)
		imm += (rex & 0x08u) ? 8u : (osz66 ? 2u : 4u);
	if (flags & F_IA)
		imm += asz67 ? 4u : 8u;
	if (flags & F_IR)
		imm += 4u;
	n += imm;

	if (n > left)
		return -1;

	in->len = n;

	if (n == SCAN_INSN_SIZE && map == MAP_0F && !vex && b == 0x05)
		in->kind = SCAN_SYSCALL;
	else if (sets_eax(op, (unsigned int)(left - (uint64_t)(op - p)), map,
	                  rex, osz66, &in->nr))
		in->kind = SCAN_SETNR;

	return 0;
}

int
scan_lookback(const uint8_t *site, uint64_t back, uint32_t *nr)
{
	uint32_t v;

	if (back >= 7 && site[-7] == 0x48 && site[-6] == 0xc7 &&
	    site[-5] == 0xc0) {
		__builtin_memcpy(&v, site - 4, 4);
		*nr = v;
		return 0;
	}

	if (back >= 6 && site[-6] == 0xc7 && site[-5] == 0xc0) {
		__builtin_memcpy(&v, site - 4, 4);
		*nr = v;
		return 0;
	}

	if (back >= 5 && site[-5] == 0xb8) {
		__builtin_memcpy(&v, site - 4, 4);
		*nr = v;
		return 0;
	}

	if (back >= 3 && site[-3] == 0x48 &&
	    (site[-2] == 0x31 || site[-2] == 0x33) && site[-1] == 0xc0) {
		*nr = 0;
		return 0;
	}
	if (back >= 2 && (site[-2] == 0x31 || site[-2] == 0x33) &&
	    site[-1] == 0xc0) {
		*nr = 0;
		return 0;
	}

	return -1;
}
