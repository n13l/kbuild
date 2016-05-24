/*
 *	UCW Library -- Fast Buffered I/O on Memory-Mapped Files
 *
 *	(c) 2002 Martin Mares <mj@ucw.cz>
 *
 *	This software may be freely distributed and used according to the terms
 *	of the GNU Lesser General Public License.
 */

#undef LOCAL_DEBUG

#include <sys/compiler.h>
#include <sys/log.h>
#include <opt/fastbuf.h>
#include <opt/conf.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static uint mmap_window_size = 16*CPU_PAGE_SIZE;
static uint mmap_extend_size = 4*CPU_PAGE_SIZE;

#ifndef TEST
static struct cf_section _unused fbmm_config = {
  CF_ITEMS {
    CF_UINT("WindowSize", &mmap_window_size),
    CF_UINT("ExtendSize", &mmap_extend_size),
    CF_END
  }
};

static void _constructor fbmm_init_config(void)
{
  //cf_declare_section("FBMMap", &fbmm_config, 0);
}
#endif

struct fb_mmap {
  struct fastbuf fb;
  int fd;
  int is_temp_file;
  off_t file_size;
  off_t file_extend;
  off_t window_pos;
  uint window_size;
  int mode;
};
#define FB_MMAP(f) ((struct fb_mmap *)(f))

static void
bfmm_map_window(struct fastbuf *f)
{
  struct fb_mmap *F = FB_MMAP(f);
  off_t pos0 = f->pos & ~(off_t)(CPU_PAGE_SIZE-1);
  int l = _min((off_t)mmap_window_size, F->file_extend - pos0);
  uint ll = _align_to(l, CPU_PAGE_SIZE);
  int prot = ((F->mode & O_ACCMODE) == O_RDONLY) ? PROT_READ : (PROT_READ | PROT_WRITE);

  sys_dbg(" ... Mapping %x(%x)+%x(%x) len=%x extend=%x", (int)pos0, (int)f->pos, ll, l, (int)F->file_size, (int)F->file_extend);
  if (ll != F->window_size && f->buffer)
    {
      munmap(f->buffer, F->window_size);
      f->buffer = NULL;
    }
  F->window_size = ll;
  if (!f->buffer)
    f->buffer = mmap(NULL, ll, prot, MAP_SHARED, F->fd, pos0);
  else
    f->buffer = mmap(f->buffer, ll, prot, MAP_SHARED | MAP_FIXED, F->fd, pos0);
  if (f->buffer == (byte *) MAP_FAILED)
    {
      f->buffer = NULL;
      bthrow(f, "mmap", "mmap(%s): ", f->name);
    }
#ifdef MADV_SEQUENTIAL
  if (ll > CPU_PAGE_SIZE)
    madvise(f->buffer, ll, MADV_SEQUENTIAL);
#endif
  f->bufend = f->buffer + l;
  f->bptr = f->buffer + (f->pos - pos0);
  F->window_pos = pos0;
}

static int
bfmm_refill(struct fastbuf *f)
{
  struct fb_mmap *F = FB_MMAP(f);

  sys_dbg("Refill <- %p %p %p %p", f->buffer, f->bptr, f->bstop, f->bufend);
  if (f->pos >= F->file_size)
    return 0;
  if (f->bstop >= f->bufend)
    bfmm_map_window(f);
  if (F->window_pos + (f->bufend - f->buffer) > F->file_size)
    f->bstop = f->buffer + (F->file_size - F->window_pos);
  else
    f->bstop = f->bufend;
  f->pos = F->window_pos + (f->bstop - f->buffer);
  sys_dbg(" -> %p %p %p(%x) %p", f->buffer, f->bptr, f->bstop, (int)f->pos, f->bufend);
  return 1;
}

static void
bfmm_spout(struct fastbuf *f)
{
  struct fb_mmap *F = FB_MMAP(f);
  off_t end = f->pos + (f->bptr - f->bstop);

  sys_dbg("Spout <- %p %p %p %p", f->buffer, f->bptr, f->bstop, f->bufend);
  if (end > F->file_size)
    F->file_size = end;
  if (f->bptr < f->bufend)
    return;
  f->pos = end;
  if (f->pos >= F->file_extend)
    {
      F->file_extend = _align_to(F->file_extend + mmap_extend_size, (off_t)CPU_PAGE_SIZE);
      if (ftruncate(F->fd, F->file_extend))
	bthrow(f, "write", "ftruncate(%s):" , f->name);
    }
  bfmm_map_window(f);
  f->bstop = f->bptr;
  sys_dbg(" -> %p %p %p(%x) %p", f->buffer, f->bptr, f->bstop, (int)f->pos, f->bufend);
}

static int
bfmm_seek(struct fastbuf *f, off_t pos, int whence)
{
  if (whence == SEEK_END)
    pos += FB_MMAP(f)->file_size;
  else
    assert(whence == SEEK_SET);
  assert(pos >= 0 && pos <= FB_MMAP(f)->file_size);
  f->pos = pos;
  f->bptr = f->bstop = f->bufend = f->buffer;	/* force refill/spout call */
  sys_dbg("Seek -> %p %p %p(%x) %p", f->buffer, f->bptr, f->bstop, (int)f->pos, f->bufend);
  return 1;
}

static void
bfmm_close(struct fastbuf *f)
{
  struct fb_mmap *F = FB_MMAP(f);
  if (f->buffer)
    munmap(f->buffer, F->window_size);
  if (!(f->flags & FB_DEAD) &&
      F->file_extend > F->file_size &&
      ftruncate(F->fd, F->file_size))
    bthrow(f, "write", "ftruncate(%s):", f->name);
  bclose_file_helper(f, F->fd, F->is_temp_file);
  xfree(f);
}

static int
bfmm_config(struct fastbuf *f, uint item, int value)
{
  int orig;

  switch (item)
    {
    case BCONFIG_IS_TEMP_FILE:
      orig = FB_MMAP(f)->is_temp_file;
      FB_MMAP(f)->is_temp_file = value;
      return orig;
    default:
      return -1;
    }
}

struct fastbuf *
bfmmopen_internal(int fd, const char *name, uint mode)
{
  int namelen = strlen(name) + 1;
  struct fb_mmap *F = xmalloc(sizeof(struct fb_mmap) + namelen);
  struct fastbuf *f = &F->fb;

  memset(F, 0, sizeof(*F));
  f->name = (char *)(byte *)(F+1);
  memcpy(f->name, name, namelen);
  F->fd = fd;
  F->file_extend = F->file_size = lseek(fd, 0, SEEK_END);
  if (F->file_size < 0)
    bthrow(f, "open", "fb-mmap: Cannot detect size of %s -- is it seekable?", name);
  if (mode & O_APPEND)
    f->pos = F->file_size;
  F->mode = mode;

  f->refill = bfmm_refill;
  f->spout = bfmm_spout;
  f->seek = bfmm_seek;
  f->close = bfmm_close;
  f->config = bfmm_config;
  return f;
}

#ifdef TEST

int main(int UNUSED argc, char **argv)
{
  struct fb_params par = { .type = FB_MMAP };
  struct fastbuf *f = bopen_file(argv[1], O_RDONLY, &par);
  struct fastbuf *g = bopen_file(argv[2], O_RDWR | O_CREAT | O_TRUNC, &par);
  int c;

  sys_dbg("Copying");
  while ((c = bgetc(f)) >= 0)
    bputc(g, c);
  bclose(f);
  sys_dbg("Seek inside last block");
  bsetpos(g, btell(g)-1333);
  bputc(g, 13);
  sys_dbg("Seek to the beginning & write");
  bsetpos(g, 1333);
  bputc(g, 13);
  sys_dbg("flush");
  bflush(g);
  bputc(g, 13);
  bflush(g);
  sys_dbg("Seek nearby & read");
  bsetpos(g, 133);
  bgetc(g);
  sys_dbg("Seek far & read");
  bsetpos(g, 133333);
  bgetc(g);
  sys_dbg("Closing");
  bclose(g);

  return 0;
}

#endif
