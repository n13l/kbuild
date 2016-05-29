#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/pool.h>
#include <mem/page.h>
#include <cpuid.h>

#define CPU_VENDOR_MAX 12

enum cpu_vendor {
	CPU_VENDOR_UNKNOWN,
	CPU_VENDOR_INTEL,
	CPU_VENDOR_AMD,
	CPU_VENDOR_CYRIX,
	CPU_VENDOR_VIA,
	CPU_VENDOR_TRANSMETA,
	CPU_VENDOR_UMC,
	CPU_VENDOR_NEXGEN,
	CPU_VENDOR_RISE,
	CPU_VENDOR_SIS,
	CPU_VENDOR_NSC,
	CPU_VENDOR_VORTEX,
	CPU_VENDOR_RDC
};

enum cpu_hypervisor {
	CPU_HYPERVISOR_UNKNOWN,
	CPU_HYPERVISOR_VMWARE,
	CPU_HYPERVISOR_XEN,
	CPU_HYPERVISOR_KVM,
	CPU_HYPERVISOR_MICROSOFT,
};

static void
cpu_vendor(void)
{
	unsigned eax, ebx, ecx, edx;
	__get_cpuid(0, &eax, &ebx, &ecx, &edx);    

	char vendor[CPU_VENDOR_MAX + 1];
	memcpy(vendor + 0, &ebx, sizeof(ebx)); 
	memcpy(vendor + sizeof(ebx), &edx, sizeof(edx));
	memcpy(vendor + sizeof(edx) + sizeof(ebx), &ecx, sizeof(ecx));
	vendor[CPU_VENDOR_MAX] = '\0';
	printf("cpu.vendor=%s\n", vendor);
}

#ifndef bit_SSE4_2
#define bit_SSE4_2 bit_SSE42
#endif

#ifndef bit_SSE4_2
#define bit_SSE4_2 bit_SSE41
#endif

void
cpu_extension(unsigned ecx)
{
	printf("cpu.sse4.2=%s\n", ecx & bit_SSE4_2 ? "yes" : "no");
	if (ecx & bit_SSE4_2)
		return;

	printf("cpu.sse4.1=%s\n", ecx & bit_SSE4_1 ? "yes" : "no");
	if ( ecx & bit_SSE4_1)
		return;

	printf("cpu.sse3=%s\n",   ecx & bit_SSE3   ? "yes" : "no");
	if (ecx & bit_SSE3)	
		return;

	printf("cpu.sse2=%s\n",   ecx & bit_SSE2   ? "yes" : "no");
	if ( ecx & bit_SSE2)
		return;

	printf("cpu.sse=%s\n", ecx & bit_SSE ? "yes" : "no");
	if (ecx & bit_SSE)
		return;
}

int
main(int argc, char *argv[])
{
	printf("platform: %s\n", CONFIG_PLATFORM);

	cpu_vendor();

	unsigned level = 1;
	unsigned eax = 0;
	unsigned ebx;
	unsigned ecx;
	unsigned edx;

	__get_cpuid(level, &eax, &ebx, &ecx, &edx);    

	printf("cpu.stepping %d\n", eax & 0xF);
	printf("cpu.model %d\n", (eax >> 4) & 0xF);
	printf("cpu.family %d\n", (eax >> 8) & 0xF);
	printf("cpu.processor type %d\n", (eax >> 12) & 0x3);
	printf("cpu.extended model %d\n", (eax >> 16) & 0xF);
	printf("cpu.extended family %d\n", (eax >> 20) & 0xFF);

	cpu_extension(ecx);

	printf("cpu.crypto crc32c=%s\n", ecx & bit_SSE4_2 ? "yes" : "no");

	printf("cpu.pagesize=%d\n", CPU_PAGE_SIZE);
	printf("cpu.struct.align=%d\n", CPU_STRUCT_ALIGN);
	printf("cpu.cacheline=%d\n",  L1_CACHE_BYTES);

	struct mempool *mp = mp_new(CPU_PAGE_SIZE);

	mp_delete(mp);
	return 0;
}
