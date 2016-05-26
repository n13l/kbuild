/*
 * A simple growing array of an arbitrary type
 *
 * Based on A simple growing array of an arbitrary type from UCW Library
 */

#ifndef _CLIB_GARY_H
#define _CLIB_GARY_H

#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/alloc.h>

struct gary {
	size_t elts;
	size_t free;
	size_t size;
	struct mem *mem;
};

#define GARY_HDR_SIZE _align_to(sizeof(struct gary), CPU_STRUCT_ALIGN)
#define gary_hdr(ptr) ((struct gary *)((byte*)(ptr) - GARY_HDR_SIZE))
#define gary_body(ptr) ((byte *)(ptr) + GARY_HDR_SIZE)

#define GARY_INIT(ptr, n) \
	(ptr) = gary_init(sizeof(*(ptr)), (n), &mem_std)

#define GARY_INIT_ZERO(ptr, n) \
	(ptr) = gary_init(sizeof(*(ptr)), (n), &mem_zeroed)

#define GARY_INIT_ALLOC(ptr, n, a) \
	(ptr) = gary_init(sizeof(*(ptr)), (n), (a))

#define GARY_INIT_SPACE(ptr, n) \
	do { GARY_INIT(ptr, n); (gary_hdr(ptr))->elts = 0; } while (0)

#define GARY_INIT_SPACE_ZERO(ptr, n) \
	do { GARY_INIT_ZERO(ptr, n); (gary_hdr(ptr))->elts = 0; } while (0)

#define GARY_INIT_SPACE_ALLOC(ptr, n, a) \
	do { GARY_INIT_ALLOC(ptr, n, a); (gary_hdr(ptr))->elts = 0; } \
	while (0)

#define GARY_FREE(ptr) gary_free(ptr)
#define GARY_SIZE(ptr) (gary_hdr(ptr)->elts)

#define GARY_RESIZE(ptr, n) ((ptr) = gary_set_size((ptr), (n)))

#define GARY_INIT_OR_RESIZE(ptr, n) \
	(ptr) = (ptr) ? gary_set_size((ptr), (n)): \
	gary_init(sizeof(*(ptr)), (n), &mem_std)

#define GARY_PUSH_MULTI(ptr, n) ({					\
  struct gary *_h = gary_hdr(ptr);					\
  typeof(*(ptr)) *_c = &(ptr)[_h->elts];				\
  size_t _n = n;							\
  _h->elts += _n;							\
  if (_h->elts > _h->free)					\
    (ptr) = gary_push_helper((ptr), _n, (byte **) &_c);			\
  _c; })

#define GARY_PUSH(ptr)         GARY_PUSH_MULTI(ptr, 1)
#define GARY_POP_MULTI(ptr, n) GARY_HDR(ptr)->num_elts -= (n)
#define GARY_POP(ptr)          GARY_POP_MULTI(ptr, 1)
#define GARY_FIX(ptr)          (ptr) = gary_fix((ptr))

void *
gary_init(size_t size, size_t elts, struct mem *);

void *
gary_set_size(void *ary, size_t n);

void *
gary_push_helper(void *array, size_t n, byte **cptr);

void *
gary_fix(void *array);

static inline void 
gary_free(void *ptr)
{
	struct gary *h = gary_hdr(ptr);
	h->mem->free(h->mem, h);
}

extern struct gary gary_empty_hdr;                                          
#define GARY_FOREVER_EMPTY gary_body(&gary_empty_hdr)

#define GARY_PUSH_GENERIC(ptr) ({					\
	struct gary *_h = gary_hdr(ptr);				\
	void *_c = (byte *)(ptr) + _h->elts++ * _h->size;	\
	if (_h->elts > _h->free)				\
		(ptr) = gary_push_helper((ptr), 1, (byte **) &_c);      \
	_c; })

#endif
