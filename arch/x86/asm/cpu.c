#include <sys/compiler.h>
#include <sys/cpu.h>
#include <sys/log.h>
#include <elf/lib.h>
#include <cpuid.h>

#include <stdlib.h>
#include <string.h>

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

static char __cpu_vendor[CPU_VENDOR_MAX + 1] = {0};

const char *
cpu_vendor(void)
{
	unsigned eax, ebx, ecx, edx;
	__get_cpuid(0, &eax, &ebx, &ecx, &edx);    

	memcpy(__cpu_vendor + 0, &ebx, sizeof(ebx)); 
	memcpy(__cpu_vendor + sizeof(ebx), &edx, sizeof(edx));
	memcpy(__cpu_vendor + sizeof(edx) + sizeof(ebx), &ecx, sizeof(ecx));
	__cpu_vendor[CPU_VENDOR_MAX] = '\0';
	printf("cpu.vendor=%s\n", __cpu_vendor);
	return __cpu_vendor;
}

#ifndef bit_SSE4_2
#define bit_SSE4_2 bit_SSE42
#endif

#ifndef bit_SSE4_1
#define bit_SSE4_1 bit_SSE41
#endif

void
cpu_dump_extension(void)
{
	unsigned eax, ebx, ecx, edx;
	__get_cpuid(1, &eax, &ebx, &ecx, &edx);    

	printf("cpu.stepping %d\n", eax & 0xF);
	printf("cpu.model %d\n", (eax >> 4) & 0xF);
	printf("cpu.family %d\n", (eax >> 8) & 0xF);
	printf("cpu.processor type %d\n", (eax >> 12) & 0x3);
	printf("cpu.extended model %d\n", (eax >> 16) & 0xF);
	printf("cpu.extended family %d\n", (eax >> 20) & 0xFF);

	printf("cpu.sse4.2=%s\n", ecx & bit_SSE4_2 ? "yes" : "no");
	printf("cpu.sse4.1=%s\n", ecx & bit_SSE4_1 ? "yes" : "no");
	printf("cpu.sse3=%s\n",   ecx & bit_SSE3   ? "yes" : "no");
	printf("cpu.sse2=%s\n",   ecx & bit_SSE2   ? "yes" : "no");
	printf("cpu.sse=%s\n", ecx & bit_SSE ? "yes" : "no");

	printf("cpu.crypto.crc32c=%s\n", ecx & bit_SSE4_2 ? "yes" : "no");
}
