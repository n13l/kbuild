#ifndef _SYS_MMAN_H_
#define _SYS_MMAN_H_

#include <sys/compiler.h>
#include <sys/types.h>

#define PROT_NONE       0
#define PROT_READ       1
#define PROT_WRITE      2
#define PROT_EXEC       4

#define MAP_FILE        0
#define MAP_SHARED      1
#define MAP_PRIVATE     2
#define MAP_TYPE        0xf
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20
#define MAP_ANON        MAP_ANONYMOUS

#define MAP_FAILED      ((void *)-1)

/* Flags for msync. */
#define MS_ASYNC        1
#define MS_SYNC         2
#define MS_INVALIDATE   4

void*
mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

int
munmap(void *addr, size_t len);

void *
mremap(void *addr, size_t size, size_t nsize, int mode);

int
mprotect(void *addr, size_t len, int prot);

int
msync(void *addr, size_t len, int flags);

int
mlock(const void *addr, size_t len);

int
munlock(const void *addr, size_t len);

#endif/*_SYS_MMAN_H_*/
