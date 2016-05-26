#include <sys/compiler.h>
#include <mem/alloc.h>
#include <generic/gary.h>
#include <string.h>

struct gary gary_empty_hdr;

void *
gary_init(size_t size, size_t elts, struct mem *mem)
{
	struct gary *h = mem->alloc(mem, GARY_HDR_SIZE + size * elts);

	h->elts = h->free = elts;
	h->size = size;
	h->mem  = mem;

	return gary_body(h);
}

static struct gary *
gary_realloc(struct gary *h, size_t n)
{
	size_t o = h->free;
	if (n > 2 * h->free)
		h->free = n;
	else
		h->free *= 2;

	size_t x = GARY_HDR_SIZE + o * h->size;
	size_t y = GARY_HDR_SIZE + h->free * h->size;

	return h->mem->realloc(h->mem, h, x, y);
}

void *
gary_set_size(void *array, size_t n)
{
	struct gary *h = gary_hdr(array);
	h->elts = n;
	if (n <= h->free)
		return array;

	h = gary_realloc(h, n);
	return gary_body(h);
}

void *
gary_push_helper(void *array, size_t n, byte **cptr)
{
	struct gary *h = gary_hdr(array);
	h = gary_realloc(h, h->elts);
	*cptr = gary_body(h) + (h->elts - n) * h->size;
	return gary_body(h);
}

void *
gary_fix(void *array)
{
	struct gary *h = gary_hdr(array);
	if (h->elts != h->free) {
		size_t o = GARY_HDR_SIZE + h->free * h->size;
		size_t n = GARY_HDR_SIZE + h->elts * h->size;
		h = h->mem->realloc(h->mem, h, o, n);
		h->free = h->elts;
	}
	return gary_body(h);
}
