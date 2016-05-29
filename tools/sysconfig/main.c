#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/pool.h>
#include <mem/page.h>
#include <cpu.h>

int
main(int argc, char *argv[])
{
	printf("platform: %s\n", CONFIG_PLATFORM);

	cpu_vendor();
	cpu_dump_extension();

	printf("cpu.pagesize=%d\n", CPU_PAGE_SIZE);
	printf("cpu.struct.align=%d\n", CPU_STRUCT_ALIGN);
	printf("cpu.cacheline=%d\n",  L1_CACHE_BYTES);

	struct mempool *mp = mp_new(CPU_PAGE_SIZE);

	mp_delete(mp);
	return 0;
}
