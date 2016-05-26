/*                                                                              
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

#ifndef __CLIB_STRING_ATTR_H__
#define __CLIB_STRING_ATTR_H__

#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/pool.h>
#include <mem/list.h>
#include <stdbool.h>
#include <string.h>

#define ATTR_MULTI   0x1
#define ATTR_CHANGED 0x2
#define ATTR_DELETED 0x4

#define ATTR_MAX_KEY_LEN 64
#define ATTR_MAX_VAL_LEN 2048

struct attr_list {
	struct node node;
	struct mempool *mp;
	struct list attrs;
	struct list childs;
	void *data;
};

struct attr {
	struct node node;
	const char *key;
	const char *val;
	int flags;
};

void *
attr_list_alloc(unsigned int size);

void *
attr_list_alloc_zero(unsigned int size);

void
attr_list_free(struct attr_list *list);

void
attr_list_merge(struct attr_list *from, struct attr_list *to);

void
attr_list_flush(struct attr_list *list);

int
attr_list_write(struct attr_list *, const char *addr, unsigned int max);

int
attr_list_read(struct attr_list *attr_list, byte *pkt,  unsigned int len);

struct attr *
attr_lookup(struct attr_list *o, const char *key, bool create);

int
attr_set(struct attr_list *o, const char *key, const char *val);

int
attr_set_fmt(struct attr_list *, const char *key, const char *fmt, ...);

int
attr_set_num(struct attr_list *, const char *key, int num);

int
attr_raw_set(struct attr_list *o, const char *key, const char *val);

int
attr_add(struct attr_list *,const char *key, const char *val);

int
attr_add_fmt(struct attr_list *, const char *key, const char *fmt, ...);

int
attr_set_raw(struct attr_list *o, const char *key, const char *val);

const char *
attr_get(struct attr_list *list, const char *key);

struct attr *
attr_find(struct attr_list *list, const char *key);

#endif
