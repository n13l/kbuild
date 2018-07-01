/*
 * Generic hooks
 *
 * The MIT License (MIT)         Copyright (c) 2017 Daniel Kubec <niel@rtfm.cz> 
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

#ifndef __GENERIC_HOOK_H__
#define __GENERIC_HOOK_H__

#include <list.h>

#ifndef OK
#define OK      0x0000
#endif

#ifndef DECLINE
#define DECLINE 0x0001
#endif

#define HOOK_FIRST   0x0001
#define HOOK_MIDDLE  0x0002
#define HOOK_LAST    0x0003

#define DEFINE_HOOK(rv,name,args)

struct hook {
	struct node n;
	const char *name;
	void(*fn)(void);
	int order;
};

int
hook_lookup(const char *name);

int
hook_sort_all(void);

int
hook_add(const char *name, void(*fn)(void), char **pre, char **succ, int order);

#endif/*__GENERIC_HOOK_H__*/
