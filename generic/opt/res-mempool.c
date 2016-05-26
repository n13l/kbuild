/*
 *	The UCW Library -- Resources for Memory Pools
 *
 *	(c) 2011 Martin Mares <mj@ucw.cz>
 *
 *	This software may be freely distributed and used according to the terms
 *	of the GNU Lesser General Public License.
 */

#include <sys/compiler.h>
#include <sys/log.h>
#include <opt/resource.h>
#include <mem/pool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
mp_res_free(struct resource *r)
{
  mp_delete(r->priv);
}

static void
mp_res_dump(struct resource *r, uint indent _unused)
{
  printf(" pool=%p\n", r->priv);
}

static const struct res_class mp_res_class = {
  .name = "mempool",
  .dump = mp_res_dump,
  .free = mp_res_free,
};

struct resource *
res_mempool(struct mempool *mp)
{
  return res_new(&mp_res_class, mp);
}
