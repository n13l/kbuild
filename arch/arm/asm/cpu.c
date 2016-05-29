#include <sys/compiler.h>
#include <sys/cpu.h>
#include <sys/log.h>
#include <elf/lib.h>

const char *
cpu_vendor(void)
{
	printf("cpu.vendor=arm\n");
	return NULL;
}

void
cpu_dump_extension(void)
{
}
