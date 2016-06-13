/*
 * Generic Single linked, Double linked and Circular doubly-linked list
 *
 * The MIT License (MIT)         
 *
 * Copyright (c) 2013 Daniel Kubec <niel@rtfm.cz>
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
 **/

#ifndef __GENERIC_SDC_LIST_H__
#define __GENERIC_SDC_LIST_H__

#include <sys/compiler.h>
#include <mem/debug.h>

struct node {
	struct node *next, *prev;
};

struct snode {
	struct snode *next;
};

struct hnode {
	struct hnode *next, **prev;
};

struct list {
	struct node head;
};

struct slist {
	struct snode *head;
};

struct hlist {
	struct hnode *head;
};

static inline void
snode_init(struct snode *snode)
{
	snode->next = NULL;
}

static inline void
slist_add_after(struct snode *node, struct snode *after)
{
	after->next = node;
}


static inline void
slist_remove(struct snode *node, struct snode *prev)
{
	if (prev)
		prev->next = node->next;
}

#define slist_for_each_safe(item, member, it) \
	for (; item && ({it = (__typeof__(item))item->member.next; 1;} ); \
	       item = it)

static inline void 
list_init(struct list *l)
{
	struct node *head = &l->head;
	head->next = head->prev = head;
}

static inline void *
list_head(struct list *l)
{
	return (l->head.next != &l->head) ? l->head.next : NULL;
}

static inline void *
list_tail(struct list *l)
{
	return (l->head.prev != &l->head) ? l->head.prev : NULL;
}

static inline void *
list_next(struct list *l, struct node *n)
{
	return (n->next != &l->head) ? (void *) n->next : NULL;
}

static inline void *
list_prev(struct list *list, struct node *node)
{
	return (node->prev != &list->head) ? (void *)node->prev : NULL;
}

static inline int
list_empty(struct list *list)
{
	return (list->head.next == &list->head);
}

static inline void
list_add_after(struct node *node, struct node *after)
{
	struct node *before = after->next;
	node->next = before;
	node->prev = after;
	before->prev = node;
	after->next = node;
}

static inline void
list_add_before(struct node *node, struct node *before)
{
	struct node *after = before->prev;
	node->next = before;
	node->prev = after;
	before->prev = node;
	after->next = node;
}

static inline void
list_add_head(struct list *list, struct node *node)
{
	list_add_after(node, &list->head);
}

static inline void
list_add_tail(struct list *list, struct node *node)
{
	list_add_before(node, &list->head);
}

#ifdef CONFIG_DEBUG_LIST
void __list_remove(struct node *node);
#else
static inline void __list_remove(struct node *node)
{
	struct node *before = node->prev;
	struct node *after  = node->next;
	before->next = after;
	after->prev = before;
}
#endif
static inline void
list_remove(struct node *node)
{
	__list_remove(node);
}

static inline void *
list_remove_head(struct list *list)
{
	struct node *node = (struct node *)list_head(list);
	if (node)
		list_remove(node);
	return node;
}

static inline void 
list_insert_list_after(struct list *list, struct node *after)
{
	if (list_empty(list))
		return;

	struct node *node = &list->head;
	node->prev->next = after->next;
	after->next->prev = node->prev;
	node->next->prev = after;
	after->next = node->next;
	list_init(list);
}

#define list_walk(n, list) \
	for (n = (void*)(list).head.next;\
		(struct node*)(n) != &(list).head;\
		n = (void*)((struct node*)(n))->next)

#define list_walk_safe(n, list, tmp) \
	for (n = (void*)(list).head.next;\
		tmp = (void*)((struct node*)(n))->next, \
		(struct node*)(n) != &(list).head;\
		n = (void*)tmp)

#define list_for_each(type, n, list) \
	for (type n = (type)(list).head.next;\
		(struct node*)(n) != &(list).head;\
		n = (type)((struct node*)(n))->next)

#define list_for_each_safe(type, n, list, tmp) \
	for (type n = (void*)(list).head.next; \
		tmp = (void*)((struct node*)(n))->next, \
		(struct node*)(n) != &(list).head;\
		n = (void*)tmp)

#define list_head_init(name) { &(name), &(name) }

#define hlist_init { .head = NULL }
#define hlist(name) struct hlist name = {  .head = NULL }
#define init_hlist(ptr) ((ptr)->head = NULL)

static inline void
init_hnode(struct hnode *hnode)
{
	hnode->next = NULL;
	hnode->prev = NULL;
}

static inline void 
hlist_add_head(struct hnode *hnode, struct hlist *hlist)
{
	struct hnode *head = hlist->head;
	hnode->next = head;
	if (head)
		head->prev = &hnode->next;
	hlist->head = hnode;
	hnode->prev = &hlist->head;
}

static inline int
hlist_unhashed(const struct hnode *hnode)
{
	return !hnode->prev;
}

static inline int
hlist_empty(const struct hlist *hlist)
{
	return !hlist->head;
}

#ifdef CONFIG_DEBUG_LIST
void __hlist_del(struct hnode *hnode);
#else
static inline void __hlist_del(struct hnode *hnode)
{
	struct hnode *next = hnode->next;
	struct hnode **prev = hnode->prev;
	*prev = next;
	if (next)
		next->prev = prev;
}
#endif
static inline void
hlist_del(struct hnode *hnode)
{
	__hlist_del(hnode);
}

static inline void
hlist_add_before(struct hnode *hnode, struct hnode *next)
{
	hnode->prev = next->prev;
	hnode->next = next;
	next->prev = &hnode->next;
	*(hnode->prev) = hnode;
}

static inline void
hlist_add_after(struct hnode *hnode, struct hnode *next)
{
	next->next = hnode->next;
	hnode->next = next;
	next->prev = &hnode->next;

	if(next->next)
		next->next->prev  = &next->next;
}

#ifdef CONFIG_DEBUG_LIST
#ifndef hlist_next_hook(pos)
#define hlist_next_hook(pos)
#endif
# define hlist_first(node)     ({ (list)->head; )}
# define hlist_next(node, pos) ({node = node->next; hlist_next_hook(pos) 1;})
#else
# define hlist_first(node)     ({ (list)->head; })
# define hlist_next(node, pos) ({node = pos->next; 1;})
#endif

#define hlist_for_each(node, list) \
	for (node = hlist_first(list); node; node = item->next)

#define hlist_for_each_safe(node, it, list) \
	for (node = hlist_first(list); it && ({it = pos->next; 1;}); node = it)

#endif
