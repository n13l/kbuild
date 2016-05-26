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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/pool.h>
#include <mem/list.h>

#include "string/attr.h"

struct attr *
attr_lookup(struct attr_list *list, const char *key, bool create)
{
	list_for_each(struct attr *, a, list->attrs)
		if (!strcmp(a->key, key))
			return a;

	if (!create)
		return NULL;

	struct attr *a = mp_alloc_zero(list->mp, sizeof(*a));
	a->key = mp_strdup(list->mp, key);
	list_add_tail(&list->attrs, &a->node);

	return a;
}

int
attr_set(struct attr_list *list, const char *key, const char *val)
{
	struct attr *a = attr_lookup(list, key, 1);
	a->val = val ? mp_strdup(list->mp, val) : NULL;
	a->flags |= ATTR_CHANGED;
	return 0;
}

int
attr_set_fmt(struct attr_list *list, const char *key, const char *fmt, ... )
{
	struct attr *a = attr_lookup(list, key, 1);

        va_list args;
        va_start(args, fmt);

	a->val = mp_vprintf(list->mp, fmt, args);
	a->flags |= ATTR_CHANGED;

	va_end(args);
	return 0;
}

int
attr_set_num(struct attr_list *list, const char *key, int num)
{
	struct attr *a = attr_lookup(list, key, 1);
	a->val = mp_printf(list->mp, "%d", num);
	a->flags |= ATTR_CHANGED;
	return 0;
}

int
attr_raw_set(struct attr_list *list, const char *key, const char *val)
{
	struct attr *a = attr_lookup(list, key, 1);
	a->val = val ? mp_strdup(list->mp, val) : NULL;
	return 0;
}

const char *
attr_get(struct attr_list *list, const char *key)
{
	struct attr *a = attr_lookup(list, key, 0);
	return a ? a->val : NULL;
}

struct attr *
attr_find(struct attr_list *list, const char *key)
{
	list_for_each(struct attr *, a, list->attrs)
		if (!strcmp(a->key, key))
			return a;
	return NULL;
}

void
attr_list_merge(struct attr_list *from, struct attr_list *to)
{
	list_for_each(struct attr *, a, from->attrs)
		attr_set(to, a->key, a->val);
}

void
attr_list_sort(struct attr_list *list)
{
	struct attr *x, *y, *z;

	for (x = list_head(&list->attrs); x;) {
		for (z = y = x; (y = list_next(&list->attrs, &y->node));)
			if (strcmp(y->key, z->key) < 0)
				z = y;
		if (x == z)
			x = list_next(&list->attrs, &x->node);
		else {
			list_remove(&z->node);
			list_insert_before(&z->node, &x->node);
		}
	}
}

char *
attr_streval(struct attr_list *list, const char *str, bool recurse)
{
	char *val = NULL;
	return val;
}
