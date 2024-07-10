
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/mman.h>
#include <dlfcn.h>
#include <link.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

#define DW_EH_PE_omit		0xffu
#define DW_EH_PE_absptr		0x00u
#define DW_EH_PE_udata2		0x02u
#define DW_EH_PE_udata4		0x03u
#define DW_EH_PE_udata8		0x04u
#define DW_EH_PE_sdata2		0x0au
#define DW_EH_PE_sdata4		0x0bu
#define DW_EH_PE_sdata8		0x0cu
#define DW_EH_PE_datarel	0x30u

#define EH_TABLE_ENC		(DW_EH_PE_datarel | DW_EH_PE_sdata4)

struct anchors {
	uint64_t	*off;
	unsigned int	n;
};

static unsigned int
enc_size(uint8_t enc)
{
	switch (enc & 0x0fu) {
	case DW_EH_PE_absptr:	return (unsigned int)sizeof(void *);
	case DW_EH_PE_udata2:
	case DW_EH_PE_sdata2:	return 2;
	case DW_EH_PE_udata4:
	case DW_EH_PE_sdata4:	return 4;
	case DW_EH_PE_udata8:
	case DW_EH_PE_sdata8:	return 8;
	default:		return 0;
	}
}

static const uint8_t *
eh_table(uintptr_t hdr, uintptr_t ihi, uint32_t *count)
{
	const uint8_t *p = (const uint8_t *)hdr;
	unsigned int skip;
	uint32_t n;

	if (hdr + 4 > ihi || p[0] != 1)
		return NULL;
	if (p[2] != DW_EH_PE_udata4 || p[3] != EH_TABLE_ENC)
		return NULL;

	skip = p[1] == DW_EH_PE_omit ? 0 : enc_size(p[1]);
	if (p[1] != DW_EH_PE_omit && !skip)
		return NULL;

	p += 4 + skip;
	if ((uintptr_t)p + 4 > ihi)
		return NULL;
	memcpy(&n, p, sizeof(n));
	p += 4;

	if (!n || (uintptr_t)p + (uint64_t)n * 8u > ihi)
		return NULL;

	*count = n;
	return p;
}

static void
anchors_of(struct anchors *a, const uint8_t *table, uint32_t count,
           uintptr_t datarel, uintptr_t lo, uintptr_t hi)
{
	uint32_t i;

	a->off = NULL;
	a->n = 0;

	a->off = malloc((size_t)count * sizeof(*a->off));
	if (!a->off)
		return;

	for (i = 0; i < count; i++) {
		int32_t rel;
		uintptr_t loc;

		memcpy(&rel, table + (size_t)i * 8u, sizeof(rel));
		loc = datarel + (uintptr_t)(intptr_t)rel;
		if (loc >= lo && loc < hi)
			a->off[a->n++] = (uint64_t)(loc - lo);
	}
}

static int
is_vdso(const char *name)
{
	return name && strstr(name, "vdso") != NULL;
}

struct walk {
	uintptr_t	self;
};

static int
image_cb(struct dl_phdr_info *info, size_t size, void *arg)
{
	uintptr_t base = (uintptr_t)info->dlpi_addr;
	uintptr_t ilo = (uintptr_t)-1, ihi = 0, ehhdr = 0;
	const struct walk *w = arg;
	const uint8_t *table = NULL;
	uint32_t count = 0;
	unsigned int i;

	(void)size;

	if (base == w->self || is_vdso(info->dlpi_name))
		return 0;

	for (i = 0; i < info->dlpi_phnum; i++) {
		const ElfW(Phdr) *ph = &info->dlpi_phdr[i];

		if (ph->p_type == PT_LOAD) {
			uintptr_t lo = base + ph->p_vaddr;

			if (lo < ilo)
				ilo = lo;
			if (lo + ph->p_memsz > ihi)
				ihi = lo + ph->p_memsz;
		} else if (ph->p_type == PT_GNU_EH_FRAME) {
			ehhdr = base + ph->p_vaddr;
		}
	}

	if (ihi <= ilo)
		return 0;
	if (ehhdr >= ilo && ehhdr < ihi)
		table = eh_table(ehhdr, ihi, &count);

	for (i = 0; i < info->dlpi_phnum; i++) {
		const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
		struct anchors a = { NULL, 0 };
		uintptr_t lo, hi;
		int prot;

		if (ph->p_type != PT_LOAD || !(ph->p_flags & PF_X))
			continue;

		lo = base + ph->p_vaddr;
		hi = lo + ph->p_filesz;
		if (hi <= lo)
			continue;

		prot = ((ph->p_flags & PF_R) ? PROT_READ : 0) |
		       ((ph->p_flags & PF_W) ? PROT_WRITE : 0) |
		       PROT_EXEC;

		if (table)
			anchors_of(&a, table, count, ehhdr, lo, hi);

		sc_arm_image(lo, hi, prot, a.off, a.n);
		free(a.off);
	}

	return 0;
}

void
sc_walk_self(void)
{
	struct walk w;
	Dl_info self;

	memset(&w, 0, sizeof(w));
	if (dladdr((void *)(uintptr_t)sc_walk_self, &self))
		w.self = (uintptr_t)self.dli_fbase;

	dl_iterate_phdr(image_cb, &w);
}
