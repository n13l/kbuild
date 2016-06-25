#include <sys/compiler.h>
#include <sys/cpu.h>
//#include <sys/abi.h>
#include <mem/alloc.h>
#include <posix/list.h>

enum abi_call_flags {
	SYM_OPT = 1,
	SYM_REQ = 2
};

struct version {
	byte major;
	byte minor;
	byte patch;
	byte devel;
};

struct symbol {
	char *name;                                                       
	void *addr;                                                             
	enum abi_call_flags require;
};


struct interface {
	struct version version;
	const char *name;
	void *symbols[];
};

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
