#include <sys/compiler.h>
#include <sys/log.h>
#include <mem/alloc.h>
#include <mem/stack.h>
#include <mem/pool.h>

int 
main(int argc, char *argv[]) 
{
	struct mm_pool *mp = mm_pool_create(NULL, CPU_PAGE_SIZE, 0);

	/* explicit stack allocation */
	_unused void *addr1 = mm_alloc(mp, 1024);
	/* implicit stack allocation */
	_unused void *addr2 = mm_alloc(1024);

	debug("stack alloc addr1=%p", addr1);
	debug("stack alloc addr2=%p", addr2);

//	_unused const char *v = mm_printf("hi");

	/*
	debug("Some arg=%s-%d", "Test1", 2);
	*/

	mm_destroy(mp);
	return 0;
}
