/*
 *	UCW Library -- Fast Buffered I/O
 *
 *	(c) 1997--2011 Martin Mares <mj@ucw.cz>
 *
 *	This software may be freely distributed and used according to the terms
 *	of the GNU Lesser General Public License.
 */

#undef LOCAL_DEBUG

#include <sys/compiler.h>
#include <sys/log.h>
#include <mem/stack.h>

#include <opt/fastbuf.h>
#include <opt/resource.h>
#include <opt/trans.h>
#include <mem/stack.h>

#include <stdio.h>
#include <stdlib.h>

void bclose(struct fastbuf *f)
{
  if (f)
    {
      bflush(f);
      res_detach(f->res);
      sys_dbg("file: closing %p",f);
      if (f->close)
	f->close(f); /* Should always free all internal resources, even if it throws an exception */
    }
}

void _noreturn
bthrow(struct fastbuf *f, const char *id, const char *fmt, ...)
{
  //sys_dbg("FB: throwing %s", full_id);
  char full_id[16];
  snprintf(full_id, sizeof(full_id), "ucw.fb.%s", id);
  assert(!(f->flags & FB_DEAD)); /* Only one bthrow() is allowed before bclose() */
  va_list args;
  va_start(args, fmt);
  if (!f->res)
    die("fastbuf %s error: %s", f->name ? : "<fb>", stk_vprintf(fmt, args));
  f->flags |= FB_DEAD;
  f->bptr = f->bstop = f->bufend; /* Reset the buffer to guard consecutive seek/read/write */
  trans_vthrow(full_id, f, fmt, args);
}

int brefill(struct fastbuf *f, int allow_eof)
{
  sys_dbg("file refill");
  assert(!(f->flags & FB_DEAD) && f->buffer <= f->bstop && f->bstop <= f->bptr && f->bptr <= f->bufend);
  if (!f->refill)
    bthrow(f, "read", "Stream not readable");
  if (f->refill(f))
    {
      assert(f->buffer <= f->bptr && f->bptr < f->bstop && f->bstop <= f->bufend);
      return 1;
    }
  else
    {
      assert(f->buffer <= f->bptr && f->bptr == f->bstop && f->bstop <= f->bufend);
      if (!allow_eof && (f->flags & FB_DIE_ON_EOF))
	bthrow(f, "eof", "Unexpected EOF");
      return 0;
    }
}

static void do_spout(struct fastbuf *f)
{
  sys_dbg("file: spout");
  assert(!(f->flags & FB_DEAD) && f->buffer <= f->bstop && f->bstop <= f->bptr && f->bptr <= f->bufend); /* Check write mode possibly with unflushed data */
  if (!f->spout)
    bthrow(f, "write", "Stream not writeable");
  f->spout(f);
  assert(f->buffer <= f->bstop && f->bstop <= f->bptr && f->bptr <= f->bufend);
}

void bspout(struct fastbuf *f)
{
  do_spout(f);
  if (f->bstop == f->bufend)
    {
      do_spout(f);
      assert(f->bstop < f->bufend);
    }
}

void bflush(struct fastbuf *f)
{
  if (f->bptr > f->bstop)
    do_spout(f);
  else
    f->bptr = f->bstop; /* XXX: Skip the rest of the reading buffer ==> it breaks the position of the FE cursor */
  sys_dbg("file: flushed");
}

static void
do_seek(struct fastbuf *f, off_t pos, int whence)
{
  bflush(f);
//  sys_dbg("FB: seeking to pos=%lld whence=%d %p %p %p %p", (long long)pos, whence, f->buffer, f->bstop, f->bptr, f->bufend);
  if (!f->seek || !f->seek(f, pos, whence))
    bthrow(f, "seek", "Stream not seekable");
  sys_dbg("file: seeked %p %p %p %p", f->buffer, f->bstop, f->bptr, f->bufend);
  assert(f->buffer <= f->bstop && f->bstop <= f->bptr && f->bptr <= f->bufend);
  if (whence == SEEK_SET)
    assert(pos == btell(f));
  else
    assert(btell(f) >= 0);
}

void
bsetpos(struct fastbuf *f, off_t pos)
{
  /* We can optimize seeks only when reading */
  if (f->bptr < f->bstop && pos <= f->pos && pos >= f->pos - (f->bstop - f->buffer)) /* If bptr == bstop, then [buffer, bstop] may be undefined */
    f->bptr = f->bstop + (pos - f->pos);
  else if (pos != btell(f))
    {
      if (pos < 0)
	bthrow(f, "seek", "Seek out of range");
      do_seek(f, pos, SEEK_SET);
    }
}

void
bseek(struct fastbuf *f, off_t pos, int whence)
{
  switch (whence)
    {
    case SEEK_SET:
      bsetpos(f, pos);
      break;
    case SEEK_CUR:
      bsetpos(f, btell(f) + pos); /* btell() is non-negative, so an overflow will always throw "Seek out of range" in bsetpos() */
      break;
    case SEEK_END:
      if (pos > 0)
	bthrow(f, "seek", "Seek out of range");
      do_seek(f, pos, SEEK_END);
      break;
    default:
      die("bseek: invalid whence=%d", whence);
    }
}

int bgetc_slow(struct fastbuf *f)
{
  if (f->bptr < f->bstop)
    return *f->bptr++;
  if (!brefill(f, 0))
    return -1;
  return *f->bptr++;
}

int bpeekc_slow(struct fastbuf *f)
{
  if (f->bptr < f->bstop)
    return *f->bptr;
  if (!brefill(f, 0))
    return -1;
  return *f->bptr;
}

int beof_slow(struct fastbuf *f)
{
  return f->bptr >= f->bstop && !brefill(f, 1);
}

void
bputc_slow(struct fastbuf *f, uint c)
{
  if (f->bptr >= f->bufend)
    bspout(f);
  *f->bptr++ = c;
}

uint
bread_slow(struct fastbuf *f, void *b, uint l, uint check)
{
  uint total = 0;
  while (l)
    {
      uint k = f->bstop - f->bptr;

      if (!k)
	{
	  brefill(f, check);
	  k = f->bstop - f->bptr;
	  if (!k)
	    break;
	}
      if (k > l)
	k = l;
      memcpy(b, f->bptr, k);
      f->bptr += k;
      b = (byte *)b + k;
      l -= k;
      total += k;
    }
  if (check && total && l)
    bthrow(f, "eof", "breadb: short read");
  return total;
}

void
bwrite_slow(struct fastbuf *f, const void *b, uint l)
{
  while (l)
    {
      uint k = f->bufend - f->bptr;

      if (!k)
	{
	  bspout(f);
	  k = f->bufend - f->bptr;
	}
      if (k > l)
	k = l;
      memcpy(f->bptr, b, k);
      f->bptr += k;
      b = (byte *)b + k;
      l -= k;
    }
}

void
bbcopy_slow(struct fastbuf *f, struct fastbuf *t, uint l)
{
  while (l)
    {
      byte *fptr, *tptr;
      uint favail, tavail, n;

      favail = bdirect_read_prepare(f, &fptr);
      if (!favail)
	{
	  if (l == ~0U)
	    return;
	  bthrow(f, "eof", "bbcopy: source exhausted");
	}
      tavail = bdirect_write_prepare(t, &tptr);
      n = _min(l, favail);
      n = _min(n, tavail);
      memcpy(tptr, fptr, n);
      bdirect_read_commit(f, fptr + n);
      bdirect_write_commit(t, tptr + n);
      if (l != ~0U)
	l -= n;
    }
}

int bconfig(struct fastbuf *f, uint item, int value)
{
  return (f->config && !(f->flags & FB_DEAD)) ? f->config(f, item, value) : -1;
}

void brewind(struct fastbuf *f)
{
  bflush(f);
  bsetpos(f, 0);
}

int bskip_slow(struct fastbuf *f, uint len)
{
  while (len)
    {
      byte *buf;
      uint l = bdirect_read_prepare(f, &buf);
      if (!l)
	return 0;
      l = _min(l, len);
      bdirect_read_commit(f, buf+l);
      len -= l;
    }
  return 1;
}

off_t
bfilesize(struct fastbuf *f)
{
  if (!f)
    return 0;
  if (!f->seek)
    return -1;
  off_t pos = btell(f);
  bflush(f);
  if (!f->seek(f, 0, SEEK_END))
    return -1;
  off_t len = btell(f);
  bsetpos(f, pos);
  return len;
}

/* Resources */

static void fb_res_detach(struct resource *r)
{
  struct fastbuf *f = r->priv;
  f->res = NULL;
}

static void fb_res_free(struct resource *r)
{
  struct fastbuf *f = r->priv;
  f->res = NULL;
  bclose(f);
}

static void fb_res_dump(struct resource *r, uint indent)
{
  struct fastbuf *f = r->priv;
  printf(" name=%s\n", f->name);
}


static const struct res_class fb_res_class = {
  .name = "fastbuf",
  .detach = fb_res_detach,
  .dump = fb_res_dump,
  .free = fb_res_free,
};

struct fastbuf *fb_tie(struct fastbuf *f)
{
  f->res = res_new(&fb_res_class, f);
  return f;
}
