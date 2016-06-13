#include <sys/compiler.h>
#include <sys/cpu.h>
#include <sys/log.h>
#include <elf/lib.h>
#include <generic/list.h>

#include <link.h>

struct linkmap_module {
	struct node n;
	struct mempool *mem;
};

void
linkmap_init(void)
{
}
