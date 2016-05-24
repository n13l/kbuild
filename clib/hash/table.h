/*
 *                                                   Statically sized Hashtable
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN    
 * THE SOFTWARE.
 *
 * Based on statically sized htable by Sasha Levin <levinsasha928@gmail.com>
 */

#ifndef _CLIB_HASH_TABLE_H
#define _CLIB_HASH_TABLE_H

#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/list.h>

#define define_hashtable(name, bits)                                         \
	struct hlist name[1 << (bits)] =                                \
			{ [0 ... ((1 << (bits)) - 1)] = hlist_head_init}

#define declare_hashtable(name, bits)                                        \
	struct hlist name[1 << (bits)]

#define define_hashtable_index(name) \
	struct hnode name

#define define_hashtable_struct(table_name) struct table_name

#define hashtable_size(name) (_array_size(name))
#define hashtable_bits(name) log2(hashtable_size(name))

#define hash_fast(val, bits)                                                  \
	(sizeof(val) <= 4 ? hash_32(val, bits) : hash_long(val, bits))

static inline void 
__hash_init(struct hlist *list, unsigned int size)
{
	for (unsigned int i = 0; i < size; i++)
		init_hlist_head(&list[i]);
}

#define hash_init(hashtable) __hash_init(hashtable, hashtable_size(hashtable))

#define hash_add(hashtable, node, key)\
	hlist_add_head(node, &hashtable[hash_min(key, hashtable_bits(hashtable))])

#define hash_add_rcu(hashtable, node, key)					\
	hlist_add_head_rcu(node, &hashtable[hash_min(key, hashtable_bits(hashtable))])

static inline bool
hash_hashed(struct hnode *node)
{
	return !hlist_unhashed(node);
}

static inline bool
__hash_empty(struct hlist *list, unsigned int size)
{
	for (unsigned int i = 0; i < size; i++)
		if (!hlist_empty(&list[i]))
			return false;

	return true;
}

#define hash_empty(htable) __hash_empty(htable, hashtable_size(htable))

static inline void
hash_del(struct hnode *node)
{
	hlist_del_init(node);
}

#define hash_for_each(name, slot, obj, member)	                              \
	for ((slot) = 0, obj = NULL; obj == NULL &&                           \
	     (slot) < hashtable_size(name); (slot)++)                         \
	     hlist_for_each_entry(obj, &name[slot], member)

#define hash_for_each_safe(name, slot, tmp, obj, member)                      \
	for ((slot) = 0, obj = NULL; obj == NULL &&                           \
	     (slot) < hashtable_size(name); (slot)++)                         \
	     hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)

#define hash_for_each_slot(name, obj, member, key) \
	hlist_for_each_entry(obj, &name[hash_fast(key, hashtable_bits(name))],\
	                     member)

#define hash_for_each_slot_safe(name, obj, tmp, member, key)	\
	hlist_for_each_entry_safe(obj, tmp,\
	                    &name[hash_min(key, hashtable_bits(name))], member)

#endif
