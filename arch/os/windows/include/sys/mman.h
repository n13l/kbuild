#ifndef OS_WINDOWS_COMPAT_SYS_MMAN_H
#define OS_WINDOWS_COMPAT_SYS_MMAN_H

/*
 * <sys/mman.h> on Windows, over VirtualAlloc and file mappings.
 *
 * Not a platform layer in the sense io/ is: this is POSIX for sources that
 * include it and call mmap(), and it sets errno like the calls it stands in
 * for. It is all static inline so that it needs no object of its own — a tree
 * that puts arch/os/windows/include on its include path (kbuild does, for every
 * Windows compile) has it, and links nothing extra for it.
 *
 * What maps exactly:
 *
 *   mmap(MAP_ANON)           VirtualAlloc(MEM_RESERVE | MEM_COMMIT). Committed
 *                            pages are demand-zero: no physical page is used
 *                            until one is touched, as on Linux.
 *   mmap(fd)                 CreateFileMapping + MapViewOfFile; MAP_PRIVATE is
 *                            FILE_MAP_COPY, which is copy-on-write as it is
 *                            there. The offset must be a multiple of the
 *                            allocation granularity (64 KiB), not of the page.
 *   munmap()                 UnmapViewOfFile for a view, VirtualFree for the
 *                            rest — MEM_RELEASE when the range is a whole
 *                            mapping, MEM_DECOMMIT when it is part of one.
 *   mprotect()               VirtualProtect.
 *   madvise(MADV_DONTNEED)   decommit and recommit with the same protection:
 *                            the physical pages go back, the addresses stay,
 *                            and the range reads back as zeroes — what the
 *                            slab's release relies on (SLAB_VM_RELEASES).
 *   madvise(MADV_FREE)       MEM_RESET: the system may discard the contents
 *                            whenever it likes, which is MADV_FREE exactly.
 *
 * What does not:
 *
 *   MAP_NORESERVE            Windows does not overcommit: a committed page is
 *                            charged against the commit limit (RAM plus page
 *                            file) when it is committed, not when it is first
 *                            written. So a large NORESERVE mapping is accepted
 *                            only if the system could back all of it — it
 *                            costs no RAM until touched, but it is not free.
 *   madvise(anything else)   a hint, accepted and ignored. MADV_HUGEPAGE has
 *                            no Windows counterpart short of MEM_LARGE_PAGES at
 *                            allocation, which needs a privilege.
 *   mremap()                 not here, and MREMAP_MAYMOVE is not defined, which
 *                            is how callers ask whether it exists.
 */

#include <stddef.h>
#include <errno.h>
#include <io.h>
#include <windows.h>

#define PROT_NONE		0x0
#define PROT_READ		0x1
#define PROT_WRITE		0x2
#define PROT_EXEC		0x4

#define MAP_SHARED		0x01
#define MAP_PRIVATE		0x02
#define MAP_FIXED		0x10
#define MAP_ANONYMOUS		0x20
#define MAP_ANON		MAP_ANONYMOUS
#define MAP_NORESERVE		0x4000

#define MAP_FAILED		((void *)-1)

#define MADV_NORMAL		0
#define MADV_RANDOM		1
#define MADV_SEQUENTIAL		2
#define MADV_WILLNEED		3
#define MADV_DONTNEED		4
#define MADV_FREE		8

#define MS_ASYNC		1
#define MS_INVALIDATE		2
#define MS_SYNC			4

static inline DWORD
mman_page_prot(int prot)
{
	switch (prot & (PROT_READ | PROT_WRITE | PROT_EXEC)) {
	case PROT_NONE:
		return PAGE_NOACCESS;
	case PROT_READ:
		return PAGE_READONLY;
	case PROT_WRITE:
	case PROT_READ | PROT_WRITE:
		return PAGE_READWRITE;
	case PROT_EXEC:
		return PAGE_EXECUTE;
	case PROT_EXEC | PROT_READ:
		return PAGE_EXECUTE_READ;
	default:
		return PAGE_EXECUTE_READWRITE;
	}
}

static inline int
mman_errno(DWORD err)
{
	switch (err) {
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
	case ERROR_COMMITMENT_LIMIT:
		return ENOMEM;
	case ERROR_ACCESS_DENIED:
		return EACCES;
	case ERROR_INVALID_HANDLE:
		return EBADF;
	default:
		return EINVAL;
	}
}

static inline void *
mman_fail(DWORD err)
{
	errno = mman_errno(err);
	return MAP_FAILED;
}

static inline void *
mmap(void *addr, size_t len, int prot, int flags, int fd, long long off)
{
	unsigned long long end = (unsigned long long)off + len;
	DWORD page = mman_page_prot(prot), access;
	HANDLE fh = INVALID_HANDLE_VALUE, map;
	void *p;

	if (!len || off < 0)
		return mman_fail(ERROR_INVALID_PARAMETER);

	if ((flags & MAP_ANONYMOUS) && !(flags & MAP_SHARED)) {
		p = VirtualAlloc((flags & MAP_FIXED) ? addr : NULL, len,
		                 MEM_RESERVE | MEM_COMMIT, page);
		/* MAP_FIXED over a range already reserved: commit into it. */
		if (!p && (flags & MAP_FIXED))
			p = VirtualAlloc(addr, len, MEM_COMMIT, page);
		return p ? p : mman_fail(GetLastError());
	}

	if (!(flags & MAP_ANONYMOUS)) {
		fh = (HANDLE)_get_osfhandle(fd);
		if (fh == INVALID_HANDLE_VALUE) {
			errno = EBADF;
			return MAP_FAILED;
		}
	}

	/*
	 * A mapping object is created at the protection the view will need: a
	 * private view of a file is copy-on-write and needs only read access to
	 * it, which is what lets MAP_PRIVATE | PROT_WRITE map a read-only file.
	 */
	if (flags & MAP_PRIVATE)
		page = (prot & PROT_EXEC) ? PAGE_EXECUTE_WRITECOPY : PAGE_WRITECOPY;
	map = CreateFileMappingW(fh, NULL, page, (DWORD)(end >> 32),
	                         (DWORD)end, NULL);
	if (!map)
		return mman_fail(GetLastError());

	access = (flags & MAP_PRIVATE) ? FILE_MAP_COPY :
	         (prot & PROT_WRITE) ? FILE_MAP_WRITE : FILE_MAP_READ;
	if (prot & PROT_EXEC)
		access |= FILE_MAP_EXECUTE;

	p = MapViewOfFileEx(map, access, (DWORD)((unsigned long long)off >> 32),
	                    (DWORD)off, len, (flags & MAP_FIXED) ? addr : NULL);
	/* The view holds the mapping object open; this handle is not needed. */
	CloseHandle(map);
	return p ? p : mman_fail(GetLastError());
}

static inline int
munmap(void *addr, size_t len)
{
	MEMORY_BASIC_INFORMATION mi;
	unsigned char *p = (unsigned char *)addr;
	size_t whole = 0;

	if (!VirtualQuery(addr, &mi, sizeof(mi)) || mi.State == MEM_FREE) {
		errno = EINVAL;
		return -1;
	}

	if (mi.Type == MEM_MAPPED)
		return UnmapViewOfFile(mi.AllocationBase) ? 0 :
		       (errno = mman_errno(GetLastError()), -1);

	/* How much of the allocation starts here and is covered by len. */
	if (mi.AllocationBase == addr) {
		void *base = mi.AllocationBase;

		while (VirtualQuery(p + whole, &mi, sizeof(mi)) &&
		       mi.AllocationBase == base)
			whole += mi.RegionSize;
	}

	if (whole && len >= whole) {
		if (VirtualFree(addr, 0, MEM_RELEASE))
			return 0;
	} else if (VirtualFree(addr, len, MEM_DECOMMIT)) {
		return 0;
	}
	errno = mman_errno(GetLastError());
	return -1;
}

static inline int
mprotect(void *addr, size_t len, int prot)
{
	DWORD old;

	if (VirtualProtect(addr, len, mman_page_prot(prot), &old))
		return 0;
	errno = mman_errno(GetLastError());
	return -1;
}

static inline int
madvise(void *addr, size_t len, int advice)
{
	MEMORY_BASIC_INFORMATION mi;

	switch (advice) {
	case MADV_DONTNEED:
		if (!VirtualQuery(addr, &mi, sizeof(mi)))
			break;
		/*
		 * A view's pages are the file's, and reading them again gives
		 * the file back whether or not they were dropped first.
		 */
		if (mi.Type != MEM_PRIVATE)
			return 0;
		if (!VirtualFree(addr, len, MEM_DECOMMIT) ||
		    !VirtualAlloc(addr, len, MEM_COMMIT, mi.Protect))
			break;
		return 0;
	case MADV_FREE:
		if (!VirtualAlloc(addr, len, MEM_RESET, PAGE_NOACCESS))
			break;
		return 0;
	default:
		return 0;
	}
	errno = mman_errno(GetLastError());
	return -1;
}

static inline int
msync(void *addr, size_t len, int flags)
{
	MEMORY_BASIC_INFORMATION mi;

	if (!VirtualQuery(addr, &mi, sizeof(mi))) {
		errno = EINVAL;
		return -1;
	}
	if (mi.Type != MEM_MAPPED || !(flags & (MS_ASYNC | MS_SYNC)))
		return 0;
	if (FlushViewOfFile(addr, len))
		return 0;
	errno = mman_errno(GetLastError());
	return -1;
}

#endif
