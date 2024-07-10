/*
 * cpucap — CPU crypto-acceleration capability probe.
 *
 * Prints the hardware instruction-set extensions that crypto code dispatches
 * on: on x86-64 the CPUID-derived OPENSSL_ia32cap_P vector, on AArch64 the
 * getauxval(AT_HWCAP)-derived OPENSSL_armcap_P word. The decode here mirrors
 * OpenSSL's runtime capability detection, so what this tool reports is what
 * the accelerated code paths in a project built with kbuild see.
 *
 * It is dependency-free (libc only) and self-contained. kbuild always compiles
 * it with the host compiler to $(objtree)/cpucap, so from the repository root
 * just run:
 *
 *     ./obj/cpucap
 *
 * It can also be compiled and run directly, without configuring a tree:
 *
 *     cc -O2 -o cpucap scripts/cpucap.c && ./cpucap
 *
 * With --env it instead prints a single machine-readable line naming the
 * "Processor family" / "ARM CPU" Kconfig entry that best matches this host.
 * kbuild's top-level Makefile probes and exports that line (once, so every
 * recursive make agrees) so the CPU_NATIVE checkbox auto-selects the matching
 * model, pulling in its predefined capabilities, via `option env`:
 *
 *     x86_64 : HOST_X86_MODEL=<MSKYLAKEX|MZEN3|MHASWELL|MZEN|MSANDYBRIDGE|GENERIC_CPU>
 *     aarch64: HOST_ARM_MODEL=<ARM_CPU_APPLE_M4|ARM_CPU_APPLE_M1|ARM_CPU_NEOVERSE_V1|
 *                              ARM_CPU_CORTEX_A72|ARM_CPU_GENERIC>
 *              HOST_ARM_HAS_DIT=<y|n>   (drives ARM_DIT under CPU_NATIVE)
 *
 * The model is picked by capability tier — vendor-aware on x86, and on aarch64
 * Apple silicon is recognised via MIDR. The compiler is still pointed at
 * -march=native/-mcpu=native, so the model only needs to convey the right
 * capability tier, not an exact microarchitecture.
 *
 * Exit status is 0 when the probe runs, 2 on an unsupported architecture
 * (0 with no output under --env, so an unknown host just leaves the var unset
 * and the family defaults to Generic).
 */
#include <stdio.h>
#include <string.h>

/* One row of the capability table: name, present flag, and the crypto that
 * uses it, so the output ties each extension to the accelerated code. */
struct cap {
	const char *name;
	int present;
	const char *used_by;
};

static void
print_table(const char *arch, const struct cap *caps, int n)
{
	int i;

	printf("cpucap — CPU crypto-acceleration capabilities\n");
	printf("architecture : %s\n\n", arch);
	printf("%-12s %-8s %s\n", "feature", "present", "used by");
	printf("--------------------------------------------------------------\n");
	for (i = 0; i < n; i++)
		printf("%-12s %-8s %s\n", caps[i].name,
		       caps[i].present ? "yes" : "no", caps[i].used_by);
}

#if defined(__x86_64__)

#include <cpuid.h>

static unsigned long long
xgetbv0(void)
{
	unsigned int lo, hi;

	__asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
	return ((unsigned long long)hi << 32) | lo;
}

/*
 * Build OPENSSL_ia32cap_P exactly as the runtime detector does:
 *   [0] leaf 1 EDX   [1] leaf 1 ECX   [2] leaf 7 EBX   [3] leaf 7 ECX
 * with the OSXSAVE/XCR0 AVX gating and the extended-leaf XOP fix applied, so
 * AVX/AVX2 are cleared unless the OS actually enabled YMM state.
 */
static void
build_ia32cap(unsigned int cap[4], int *ymm_enabled)
{
	unsigned int eax, ebx, ecx, edx;
	unsigned int leaf1_ecx;

	cap[0] = cap[1] = cap[2] = cap[3] = 0;
	*ymm_enabled = 0;

	if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
		return;
	leaf1_ecx = ecx;
	cap[0] = edx;
	cap[1] = ecx;

	if ((leaf1_ecx >> 27) & 1)
		*ymm_enabled = (xgetbv0() & 0x6) == 0x6;

	if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
		cap[2] = ebx;
		cap[3] = ecx;
	}

	/* Derive the XOP bit (word 1 bit 11) from extended leaf 0x80000001, not
	 * leaf-1 ECX (which is SDBG there); matches cap.c. */
	cap[1] &= ~(1u << 11);
	if (__get_cpuid(0x80000001, &eax, &ebx, &ecx, &edx) && (ecx & (1u << 11)))
		cap[1] |= (1u << 11);

	if (!*ymm_enabled) {
		cap[1] &= ~(1u << 28); /* AVX  */
		cap[2] &= ~(1u << 5);  /* AVX2 */
	}
}

static void
cpu_brand(char *vendor, char *brand)
{
	unsigned int eax, ebx, ecx, edx;
	unsigned int buf[13];

	vendor[0] = brand[0] = '\0';

	if (__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
		memcpy(vendor + 0, &ebx, 4);
		memcpy(vendor + 4, &edx, 4);
		memcpy(vendor + 8, &ecx, 4);
		vendor[12] = '\0';
	}
	if (__get_cpuid(0x80000000, &eax, &ebx, &ecx, &edx) && eax >= 0x80000004) {
		unsigned int leaf;
		for (leaf = 0; leaf < 3; leaf++) {
			__get_cpuid(0x80000002 + leaf, &eax, &ebx, &ecx, &edx);
			buf[leaf * 4 + 0] = eax;
			buf[leaf * 4 + 1] = ebx;
			buf[leaf * 4 + 2] = ecx;
			buf[leaf * 4 + 3] = edx;
		}
		memcpy(brand, buf, 48);
		brand[48] = '\0';
	}
}

/*
 * Map the host to the closest "Processor family" Kconfig choice entry by
 * capability tier (AVX/AVX2/SHA-NI already reflect OS YMM-state). Vendor-aware
 * so the selected model reads naturally; -march=native handles the actual
 * codegen, so only the capability tier the model implies matters.
 */
static const char *
x86_model(const unsigned int cap[4], const char *vendor)
{
	int avx   = !!(cap[1] & (1u << 28));
	int avx2  = !!(cap[2] & (1u <<  5));
	int shani = !!(cap[2] & (1u << 29));
	int amd   = !strcmp(vendor, "AuthenticAMD");

	if (avx2 && shani)
		return amd ? "MZEN3" : "MSKYLAKEX";
	if (avx2)
		return amd ? "MZEN" : "MHASWELL";
	if (avx)
		return "MSANDYBRIDGE";
	return "GENERIC_CPU";
}

static int
run_x86(int env)
{
	unsigned int cap[4];
	int ymm;
	char vendor[13], brand[49];

	build_ia32cap(cap, &ymm);
	cpu_brand(vendor, brand);

	if (env) {
		printf("HOST_X86_MODEL=%s\n", x86_model(cap, vendor));
		return 0;
	}

	struct cap rows[] = {
		{ "SSE2",       !!(cap[0] & (1u << 26)), "baseline (all x86-64)" },
		{ "SSSE3",      !!(cap[1] & (1u <<  9)), "SHA, ChaCha20, byte-shuffle" },
		{ "PCLMULQDQ",  !!(cap[1] & (1u <<  1)), "GHASH (AES-GCM)" },
		{ "AES-NI",     !!(cap[1] & (1u << 25)), "AES-CBC / AES-GCM" },
		{ "AVX",        !!(cap[1] & (1u << 28)), "AVX digest / cipher paths" },
		{ "AVX2",       !!(cap[2] & (1u <<  5)), "AVX2 SHA / ChaCha20 paths" },
		{ "AVX-512F",   !!(cap[2] & (1u << 16)), "AVX-512 paths" },
		{ "SHA-NI",     !!(cap[2] & (1u << 29)), "SHA-1 / SHA-256 hardware" },
		{ "VAES",       !!(cap[3] & (1u <<  9)), "vectorized AES" },
		{ "VPCLMULQDQ", !!(cap[3] & (1u << 10)), "vectorized GHASH" },
		{ "RDRAND",     !!(cap[1] & (1u << 30)), "hardware RNG" },
		{ "RDSEED",     !!(cap[2] & (1u << 18)), "hardware RNG seed" },
	};

	print_table("x86_64", rows, (int)(sizeof(rows) / sizeof(rows[0])));

	printf("\n");
	if (vendor[0])
		printf("vendor            : %s\n", vendor);
	if (brand[0])
		printf("brand             : %s\n", brand);
	printf("detected model    : %s   (CPU_NATIVE selects this family)\n",
	       x86_model(cap, vendor));
	printf("OS AVX state      : %s\n", ymm ? "enabled (XCR0 YMM)"
					       : "disabled (AVX/AVX2 masked off)");
	printf("OPENSSL_ia32cap_P : 0x%08x 0x%08x 0x%08x 0x%08x\n",
	       cap[0], cap[1], cap[2], cap[3]);
	return 0;
}

#elif defined(__aarch64__)

#include <sys/auxv.h>

/* HWCAP bits — mirror OpenSSL's detector so the report matches OPENSSL_armcap_P. */
#ifndef HWCAP_ASIMD
#define HWCAP_ASIMD  (1 << 1)
#endif
#ifndef HWCAP_AES
#define HWCAP_AES    (1 << 3)
#endif
#ifndef HWCAP_PMULL
#define HWCAP_PMULL  (1 << 4)
#endif
#ifndef HWCAP_SHA1
#define HWCAP_SHA1   (1 << 5)
#endif
#ifndef HWCAP_SHA2
#define HWCAP_SHA2   (1 << 6)
#endif
#ifndef HWCAP_CRC32
#define HWCAP_CRC32  (1 << 7)
#endif
#ifndef HWCAP_SHA3
#define HWCAP_SHA3   (1 << 17)
#endif
#ifndef HWCAP_SHA512
#define HWCAP_SHA512 (1 << 21)
#endif
#ifndef HWCAP_SVE
#define HWCAP_SVE    (1 << 22)
#endif
#ifndef HWCAP_DIT
#define HWCAP_DIT    (1 << 24)   /* Data-Independent Timing (ARMv8.4) */
#endif

/* AT_HWCAP2 bits (aarch64). */
#ifndef HWCAP2_SVE2
#define HWCAP2_SVE2  (1 << 1)
#endif
#ifndef HWCAP2_RNG
#define HWCAP2_RNG   (1 << 16)   /* RNDR/RNDRRS hardware RNG (ARMv8.5) */
#endif

/*
 * MIDR_EL1 CPU identification, read from sysfs, to recognise the CPU
 * implementer for the native model mapping — notably Apple silicon.
 */
#define MIDR_IMPLEMENTER(m) (((m) >> 24) & 0xff)
#define MIDR_IMPL_APPLE     0x61

static unsigned long
read_midr(void)
{
	unsigned long midr = 0;
	FILE *f = fopen("/sys/devices/system/cpu/cpu0/regs/identification/midr_el1", "r");

	if (f) {
		if (fscanf(f, "%lx", &midr) != 1)
			midr = 0;
		fclose(f);
	}
	return midr;
}

/* OpenSSL OPENSSL_armcap_P bits. */
#define ARMV7_NEON   (1 << 0)
#define ARMV8_AES    (1 << 2)
#define ARMV8_SHA1   (1 << 3)
#define ARMV8_SHA256 (1 << 4)
#define ARMV8_PMULL  (1 << 5)
#define ARMV8_SHA512 (1 << 6)
#define ARMV8_SHA3   (1 << 11)

/*
 * Map the host to the closest "ARM CPU" Kconfig choice entry. Apple silicon is
 * recognised via MIDR (implementer 0x61) and mapped to an Apple M-series entry;
 * otherwise pick by capability tier: SHA-3 (+crypto) -> a Neoverse-class core,
 * the ARMv8 crypto set -> a crypto-capable Cortex, else Generic. -mcpu=native
 * handles codegen, so only the capability tier the model implies matters.
 */
static const char *
arm_model(unsigned long hwcap, unsigned long midr)
{
	int crypto = (hwcap & HWCAP_AES) && (hwcap & HWCAP_PMULL) &&
		     (hwcap & HWCAP_SHA1) && (hwcap & HWCAP_SHA2);
	int sha3 = crypto && (hwcap & HWCAP_SHA3);

	if (MIDR_IMPLEMENTER(midr) == MIDR_IMPL_APPLE)
		return sha3 ? "ARM_CPU_APPLE_M4" : "ARM_CPU_APPLE_M1";
	if (sha3)
		return "ARM_CPU_NEOVERSE_V1";
	if (crypto)
		return "ARM_CPU_CORTEX_A72";
	return "ARM_CPU_GENERIC";
}

static int
run_arm(int env)
{
	unsigned long hwcap = getauxval(AT_HWCAP);
	unsigned long hwcap2 = getauxval(AT_HWCAP2);
	unsigned long midr = read_midr();
	unsigned int armcap = ARMV7_NEON;

	if (env) {
		/* DIT is reported separately so ARM_DIT follows the actual host. */
		int dit = !!(hwcap & HWCAP_DIT);

		printf("HOST_ARM_MODEL=%s\n", arm_model(hwcap, midr));
		printf("HOST_ARM_HAS_DIT=%s\n", dit ? "y" : "n");
		return 0;
	}

	if (hwcap & HWCAP_AES)
		armcap |= ARMV8_AES;
	if (hwcap & HWCAP_PMULL)
		armcap |= ARMV8_PMULL;
	if (hwcap & HWCAP_SHA1)
		armcap |= ARMV8_SHA1;
	if (hwcap & HWCAP_SHA2)
		armcap |= ARMV8_SHA256;
	if (hwcap & HWCAP_SHA512)
		armcap |= ARMV8_SHA512;
	if (hwcap & HWCAP_SHA3)
		armcap |= ARMV8_SHA3;

	struct cap rows[] = {
		{ "NEON/ASIMD", !!(hwcap & HWCAP_ASIMD),  "baseline SIMD" },
		{ "AES",        !!(hwcap & HWCAP_AES),    "AES-CBC / AES-GCM" },
		{ "PMULL",      !!(hwcap & HWCAP_PMULL),  "GHASH (AES-GCM)" },
		{ "SHA-1",      !!(hwcap & HWCAP_SHA1),   "SHA-1" },
		{ "SHA-256",    !!(hwcap & HWCAP_SHA2),   "SHA-256" },
		{ "SHA-512",    !!(hwcap & HWCAP_SHA512), "SHA-512" },
		{ "SHA-3",      !!(hwcap & HWCAP_SHA3),   "SHA-3 / SHAKE" },
		{ "CRC32",      !!(hwcap & HWCAP_CRC32),  "CRC32" },
		{ "DIT",        !!(hwcap & HWCAP_DIT),    "constant-time crypto (data-independent timing)" },
		{ "RNG",        !!(hwcap2 & HWCAP2_RNG),  "hardware RNG (RNDR/RNDRRS)" },
		{ "SVE",        !!(hwcap & HWCAP_SVE),    "Scalable Vector Extension" },
		{ "SVE2",       !!(hwcap2 & HWCAP2_SVE2), "SVE2 (vector crypto base)" },
	};

	print_table("aarch64", rows, (int)(sizeof(rows) / sizeof(rows[0])));

	printf("\n");
	if (MIDR_IMPLEMENTER(midr) == MIDR_IMPL_APPLE)
		printf("implementer       : Apple silicon (MIDR 0x%08lx)\n", midr);
	else if (midr)
		printf("implementer       : 0x%02lx (MIDR 0x%08lx)\n",
		       MIDR_IMPLEMENTER(midr), midr);
	printf("detected model    : %s   (CPU_NATIVE selects this core)\n",
	       arm_model(hwcap, midr));
	printf("AT_HWCAP          : 0x%lx\n", hwcap);
	printf("AT_HWCAP2         : 0x%lx\n", hwcap2);
	printf("OPENSSL_armcap_P  : 0x%08x\n", armcap);
	return 0;
}

#endif

static void
usage(const char *argv0)
{
	printf("usage: %s [-h] [--env]\n", argv0);
	printf("  Report CPU instruction-set extensions used by crypto\n"
	       "  hardware-accelerated code paths.\n");
	printf("  --env   emit the best-matching CPU model as HOST_<arch>_MODEL=...\n"
	       "          for the build system (drives the CPU_NATIVE checkbox).\n");
}

int
main(int argc, char **argv)
{
	int env = 0;

	if (argc > 1) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			usage(argv[0]);
			return 0;
		}
		if (!strcmp(argv[1], "-e") || !strcmp(argv[1], "--env"))
			env = 1;
	}

#if defined(__x86_64__)
	return run_x86(env);
#elif defined(__aarch64__)
	return run_arm(env);
#else
	/* Under --env stay silent and succeed so an unknown host simply
	 * leaves the HOST_* vars unset (dependent features default off). */
	if (env)
		return 0;
	fprintf(stderr, "cpucap: unsupported architecture "
			"(only x86_64 and aarch64 are probed)\n");
	return 2;
#endif
}
