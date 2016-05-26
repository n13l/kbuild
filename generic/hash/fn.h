#ifndef __GENERIC_HASH_FN_H__
#define __GENERIC_HASH_FN_H__

#include <sys/compiler.h>
#include <sys/cpu.h>
#include <sys/missing.h>
#include <limits.h>

#ifndef BITS_PER_LONG

#ifdef CPU_ARCH_BITS
#define BITS_PER_LONG CPU_ARCH_BITS
#else

#if UINT_MAX == UINT32_MAX
#define BITS_PER_LONG 32
#endif
#if UINT_MAX == UINT64_MAX
#define BITS_PER_LONG 64
#endif

#endif

#endif

/* It should be prime with the word size. */
#define HASH_SHIFT_BITS     7

/*
 * Knuth recommends primes in approximately HASH ratio to the maximum
 * integer representable by a machine word for multiplicative hashing.
 * Chuck Lever verified the effectiveness of this technique:
 * http://www.citi.umich.edu/techreports/reports/citi-tr-00-1.pdf
 *
 * These primes are chosen to be bit-sparse, that is operations on
 * them can use shifts and additions instead of multiplications for
 * machines where multiplications are slow.
 */

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define HASH_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define HASH_RATIO_PRIME_64 0x9e37fffffffc0001UL

#if BITS_PER_LONG == 32
#define HASH_RATIO_PRIME HASH_RATIO_PRIME_32
#define hash_long(val, bits) hash_32((u32)val, bits)
#elif BITS_PER_LONG == 64
#define hash_long(val, bits) hash_64((u64)val, bits)
#define HASH_RATIO_PRIME HASH_RATIO_PRIME_64
#else
#error Wordsize not 32 or 64
#endif

__BEGIN_DECLS

static inline u64
hash_64(u64 val, unsigned int bits)
{
	u64 hash = val;
	u64 n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;
	/* High bits are more random, so use them. */
	return hash >> (64 - bits);
}

static inline u32
hash_32(u32 val, unsigned int bits)
{
	/* On some cpus multiply is faster, on others compiler will do shifts */
	u32 hash = val * HASH_RATIO_PRIME_32;
	/* High bits are more random, so use them. */
	return hash >> (32 - bits);
}

static inline unsigned long
hash_ptr(const void *ptr, unsigned int bits)
{
	return (unsigned long)hash_long(ptr, bits);
}

static inline unsigned int
hash_string_len_uint(unsigned int x)
{
	const unsigned int sub = ~0U / 0xff;
	const unsigned int an = sub * 0x80;
	unsigned int a = ~x & (x - sub) & an;

	if (!a)
		return sizeof(unsigned int);

	byte *bytes = (byte *) &x;
	unsigned int i;
	for (i = 0; i < sizeof(unsigned int) && bytes[i]; i++) {};
	return i;
}

static inline unsigned int
hash_string_aligned(const char *str)
{
	const unsigned int *u = (const unsigned int *)str;
	static unsigned int mhb[sizeof(unsigned int)];

	unsigned int hash = 0;
	while (1) {
		unsigned int last_len = hash_string_len_uint(*u);
		hash = _rol(hash, HASH_SHIFT_BITS);
		if (last_len < sizeof(unsigned int)) {
			unsigned int tmp = *u & mhb[last_len];
			hash ^= tmp;
			return hash;
		}
		hash ^= *u++;
        }
}

static inline unsigned int
hash_string(const char *str)
{
        const byte *s = (const byte *)str;
        unsigned int shift = _unaligned_part(s, unsigned int);
        if (!shift)
                return hash_string_aligned((const char *)s);

	unsigned int hash = 0;
	for (unsigned int i = 0; ; i++) {
		unsigned int modulo = i % sizeof(unsigned int);
#ifdef  CPU_LITTLE_ENDIAN
		unsigned int shift = modulo;
#else
		unsigned int shift = sizeof(unsigned int) - 1 - modulo;
#endif
		if (!modulo)
			hash = _rol(hash, HASH_SHIFT_BITS);
		if (!s[i])
			break;
		hash ^= s[i] << (shift * 8);
	}

	return hash;
}

__END_DECLS

#endif
