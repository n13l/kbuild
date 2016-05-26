/*
 * (c) 1997-2013 Daniel Kubec <niel@rtfm.cz>
 *
 * This software may be freely distributed and used according to the terms
 * of the GNU Lesser General Public License.
 */

#include <sys/compiler.h>
#include <sys/missing.h>
#include <ctypes/lib.h>
#include <ctypes/unicode.h>
#include <ctypes/list.h>
#include <ctypes/object.h>
#include <mem/pool.h>
#include <stdio.h>
#include <string.h>

static void
object_init(struct object *o)
{
    list_init(&o->attrs);
    list_init(&o->childs);
}

void *
object_alloc(unsigned int size)
{
    struct mempool *mp = mp_new(CPU_PAGE_SIZE);
    struct object *o = malloc(_max(size, sizeof(*o)));
    o->mp = mp;
    object_init(o);
    return o;
}

struct object *
object_new(void)
{
    struct object *o = object_alloc(sizeof(*o));
    object_init(o);
    return o;
}

void *
object_alloc_zero(unsigned int size)
{
    struct mempool *mp = mp_new(CPU_PAGE_SIZE);
    struct object *o = malloc(_max(size, sizeof(*o)));
    memset(o, 0, sizeof(*o));
    o->mp = mp;
    object_init(o);
    return o;
}

void
object_flush(void *o)
{
    struct object *object = (struct object *)o;
    mp_flush(object->mp);
    object_init(o);
}

void
object_free(void *o)
{
    struct object *object = (struct object *)o;
    mp_delete(object->mp);
    free(object);
}

void
object_merge(struct object *from, struct object *to)
{
    list_for_each(struct attr *, a, from->attrs)
        attr_set(to, a->key, a->val);
}

void
object_sort(struct object *o)
{
    struct object *object = (struct object *)o;
    struct attr *x, *y, *z;
    for (x = list_head(&o->attrs); x;) {
        for (z = y = x; (y = list_next(&o->attrs, &y->node)); )
            if (strcmp(y->key, z->key) < 0)
        z = y;

        if (x == z)
            x = list_next(&o->attrs, &x->node);
        else {
            list_remove(&z->node);
            list_insert_before(&z->node, &x->node);
        }
}
}

static int
valid_key_p(const byte *key)
{
    const byte *a = key;
    unsigned int c;

    while ((c = *a++)) {
      if (c == '.')
        {
          // Double dots, final dots and initial dots are forbidden
          if (!a[0] || a[0] == '.' || a == key+1)
            return 0;
        }
      else if (! ((c >= 'a' && c <= 'z') ||
                  (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  (c == '-')))
        {
          // No other characters allowed, the only exception is "[]" at the end
          if (!(a > key+1 && a[-2] != '.' && c == '[' && a[0] == ']' && !a[1]))
            return 0;
          a++;
        }
    }
  return (a-key <= ATTR_MAX_KEY_LEN + 1);
}

static int
valid_val_p(const byte *v)
{
return 0;
  const byte *start = v;
  for (;;)
    {
      uint c;
      //v = utf8_32_get_repl(v, &c, ~0U);
      if (!c)
        return (v-start <= ATTR_MAX_VAL_LEN + 1);
      if (c == ~0U ||
          (c < 0x20 && c != '\t') ||
          c == 0x7f)
        return 0;
    }
}

int
object_write(struct object *object, const char *addr, unsigned int size)
{
    char *p = (char *)addr;
    unsigned int len = 0;
    list_for_each(struct attr *, a, object->attrs) {
        int rv = snprintf(p, size - len - 1, "%s: %s\n", a->key, a->val);
        p += rv;
        len += rv;
        *p = '\n';
    }

    return len;
}

int
object_read(struct object *object, byte *pkt, unsigned int len)
{
    byte *start = pkt, *end = pkt + len;

    while (pkt < end) {
		byte *key = pkt;
		while (pkt < end && *pkt != ':' && *pkt != '\n')
			pkt++;

		if (pkt >= end)
			return -1;

		if (*pkt != ':')
			return pkt - start;

		*pkt++ = 0;

                while (pkt < end && *pkt == ' ')
                        pkt++;

		byte *value = pkt;

		while (pkt < end && *pkt != '\n')
			pkt++;

		if (pkt >= end)
			return -1;
		*pkt++ = 0;
/*
		if (!valid_key_p(key) || !valid_val_p(value))
			return -1;
*/
		attr_set(object, (const char *)key, (const char *)value);
	}

	return len;
}

#ifdef TEST

int main(UNUSED int argc, UNUSED char *argv[])
{
	struct list list;
	list_init(&list);

	for (int i = 0; i < 10; i++) {
		struct object *object = object_alloc(0);
		list_add_tail(&list, &object->node);

		object_init(object);
		object->node.id = i + 1;
	}

	void *tmp;
	list_for_each_delsafe(struct object *, o, list, tmp) {
		list_remove(&o->node);
		object_free(o);
	}

    return EXIT_SUCCESS;
}

#endif
