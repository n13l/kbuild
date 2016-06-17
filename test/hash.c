#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/alloc.h>
#include <mem/cache.h>
#include <posix/list.h>
#include <posix/hash.h>

DEFINE_HASHTABLE(object, 9);

int 
main(int argc, char *argv[]) 
{
	//void *addr = mm_alloc(MM_HEAP, CPU_PAGE_SIZE);

	//free(addr);
	return 0;
}
