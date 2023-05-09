#include <zephyr/kernel.h>
#include <zephyr/kernel/obj_core.h>

static struct k_spinlock  lock;

sys_dlist_t obj_type_list = SYS_DLIST_STATIC_INIT(&obj_type_list);

void z_obj_type_init(struct k_obj_type *type, uint32_t id, size_t off)
{
	sys_dlist_init(&type->list);
	sys_dlist_append(&obj_type_list, &type->node);
	type->id = id;
	type->obj_core_offset = off;
}

void k_obj_core_init(struct k_obj_core *obj_core, struct k_obj_type *type)
{
	__ASSERT((obj_core != NULL) && (type != NULL), "NULL parameter");

	sys_dnode_init(&obj_core->node);
	obj_core->type = type;
}

void k_obj_core_link(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key = k_spin_lock(&lock);
	sys_dlist_append(&obj_core->type->list, &obj_core->node);
	k_spin_unlock(&lock, key);
}

void k_obj_core_unlink(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key = k_spin_lock(&lock);
	if (sys_dnode_is_linked(&obj_core->node)) {
		sys_dlist_remove(&obj_core->node);
	}
	k_spin_unlock(&lock, key);
}

struct k_obj_type *k_obj_type_find(uint32_t type_id)
{
	struct k_obj_type *type;
	struct k_obj_type *rv = NULL;
	sys_dnode_t *node;

	k_spinlock_key_t  key = k_spin_lock(&lock);

	SYS_DLIST_FOR_EACH_NODE(&obj_type_list, node) {
		type = CONTAINER_OF(node, struct k_obj_type, node);
		if (type->id == type_id) {
		    rv = type;
		    break;
		}
	}
	
	k_spin_unlock(&lock, key);

	return rv;
}

int k_obj_type_walk_locked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *, void *),
			   void *data)
{
	k_spinlock_key_t  key;
	struct k_obj_core *obj_core;
	sys_dnode_t *node;
	int  status = 0;

	key = k_spin_lock(&lock);

	SYS_DLIST_FOR_EACH_NODE(&type->list, node) {
		obj_core = CONTAINER_OF(node, struct k_obj_core, node);
		status = func(obj_core, data);
		if (status != 0) {
			break;
		}
	}

	k_spin_unlock(&lock, key);

	return status;
}

int k_obj_type_walk_unlocked(struct k_obj_type *type,
			   int (*func)(struct k_obj_core *, void *),
			   void *data)
{
	k_spinlock_key_t  key;
	struct k_obj_core *obj_core;
	sys_dnode_t *node;
	sys_dnode_t *next;
	int  status = 0;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&type->list, node, next) {
		key = k_spin_lock(&lock);
		obj_core = CONTAINER_OF(node, struct k_obj_core, node);
		status = func(obj_core, data);
		k_spin_unlock(&lock, key);
		if (status != 0) {
			break;
		}
	}

	return status;
}
