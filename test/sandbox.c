#include <sys/compiler.h>
#include <sys/cpu.h>
//#include <sys/abi.h>
#include <mem/alloc.h>

#define DEFINE_INTERFACE(name)

DEFINE_INTERFACE(system);

/*
DEFINE_INTERFACE(system) {
};

DECLARE_INTERFACE(system);

DECLARE_INTERFACE_VERSION(0.0.1) \
*/

int
hihack_system(const char *command);

int 
main(int argc, char *argv[]) 
{
	//linkmap_add_interface();
	//linkmap_del_interface();
	return 0;
}
