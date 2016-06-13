#include <sys/compiler.h>
#include <mem/alloc.h>
#include <mem/stack.h>

int 
main(int argc, char *argv[]) 
{
	/* explicit stack allocation */
	_unused void *addr1 = mm_alloc(MM_STACK, 1024);
	/* implicit stack allocation */
	_unused void *addr2 = mm_alloc(1024);

	_unused void *addr3 = sp_alloc(CPU_CACHE_LINE);

	_unused const char *v = mm_printf(MM_STACK, "hi");
	return 0;
}
