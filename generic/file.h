/*
 * Generic file interface over several platforms
 *
 * The MIT License (MIT)         Copyright (c) 2015 Daniel Kubec <niel@rtfm.cz> 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"),to deal
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __CLIB_FILE_H__
#define __CLIB_FILE_H__

#include <stdlib.h>
#include <stdarg.h>

/* A private structure containing the file object */
struct file;
struct pollfd;

int
sys_open(struct file *file, const char *name, int flags);

int
sys_read(struct file *file, const byte *, size_t size);

int
sys_write(struct file *file, const byte *, size_t size);

int
sys_seek(struct file *file, off_t pos, int whence);

int
sys_poll(struct file *file, struct pollfd *fds, int nfds, int timeout);

int
sys_socket(struct file *file, int domain, int type, int protocol);

int
sys_bind(struct file *file, struct sockaddr *addr, size_t size);

void
sys_close(struct file *file);

#endif/*__CLIB_FILE_LIB_H__*/
