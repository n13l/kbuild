/* This file contains the linked list implementations for DEBUG_LIST. */

#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/alloc.h>
#include <generic/list.h>

void 
__list_remove(struct node *node)
{
	struct node *before = node->prev;
	struct node *after = node->next;
	before->next = after;
	after->prev = before;

	node->next = MM_ADDR_POISON1;
	node->prev = MM_ADDR_POISON2;
}
}

void
__hlist_del(struct hnode *hnode)
{
	struct hnode *next = hnode->next;
	struct hnode **prev = hnode->prev;
	*prev = next;
	if (next)
		next->prev = prev;

	hnode->next = (struct hnode * )MM_ADDR_POISON1;
	hnode->prev = (struct hnode **)MM_ADDR_POISON2;
}
