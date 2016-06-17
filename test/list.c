#include <sys/compiler.h>
#include <sys/cpu.h>
#include <mem/alloc.h>
#include <mem/cache.h>
#include <posix/list.h>

struct person {
	const char *name;
	struct node node;
};

static void
test1_list(void)
{
	DECLARE_LIST(list);

	struct person daniel  = { .name = "Daniel",  .node = init_node };
	struct person daniela = { .name = "Daniela", .node = init_node };
	struct person adam    = { .name = "Adam",    .node = init_node };
	struct person eve     = { .name = "Eve",     .node = init_node };
	struct person robot   = { .name = "Robot",   .node = init_node };

	list_add_tail(&list, &daniel.node);
	list_add_tail(&list, &daniela.node);
	list_add_tail(&list, &adam.node);
	list_add_tail(&list, &eve.node);
	list_add_tail(&list, &robot.node);

	/* iterate over all objects */
	list_for_each(n, list) {
		struct person *p = container_of(n, struct person, node);
		debug("node=%p person=%p name=%s", n, p, p->name);
	}

	/* iterate and unlink adam */
	struct node *it;
	list_for_each_delsafe(n, list, it) {
		struct person *p = container_of(n, struct person, node);
		if (!strcasecmp(p->name, "Adam"))
			list_del(&p->node);
		debug("node=%p it=%p person=%p name=%s", n, it, p, p->name);
	}

	/* iterate over all objects */
	list_for_each(n, list) {
		struct person *p = container_of(n, struct person, node);
		debug("node=%p person=%p name=%s", n, p, p->name);
	}

	struct person *p = NULL;
	/* iterate over rest: starts at daniela node */
	list_walk(n, list, &daniela.node) {
		p = container_of(n, struct person, node);
		debug("node=%p it=%p person=%p name=%s", n, it, p, p->name);
		break;
	}
	/* iterate over rest with del safety: starts at daniel node */
	list_walk_delsafe(n, list, p->node.next, it) {
		p = container_of(n, struct person, node);
		debug("node=%p it=%p person=%p name=%s", n, it, p, p->name);
	}


}

static void
test2_list(void)
{
	_unused DECLARE_LIST(list);
}


int 
main(int argc, char *argv[]) 
{
	test1_list();
	test2_list();
	return 0;
}