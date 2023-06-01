#ifndef __KERNEL_OBJ_CORE_H__
#define __KERNEL_OBJ_CORE_H__

#include <zephyr/sys/dlist.h>

/**
 * @brief Macro to convert kernel object into pointer to object core
 *
 * This macro is used to convert a pointer to a kernel object (e.g. thread or
 * memory block) into a pointer to its object core.
 */
#define K_OBJ_CORE(kobj)  &((kobj)->obj_core)

/* Known kernel object types */

#define OBJ_TYPE_CONDVAR_ID    0x434f4e44   /* "COND" in ASCII */
#define OBJ_TYPE_CPU_ID        0x4350555f   /* "CPU_" in ASCII */
#define OBJ_TYPE_EVENT_ID      0x45564e54   /* "EVNT" in ASCII */
#define OBJ_TYPE_FIFO_ID       0x4649464f   /* "FIFO" in ASCII */
#define OBJ_TYPE_KERNEL_ID     0x4B524e4c   /* "KRNL" in ASCII */
#define OBJ_TYPE_LIFO_ID       0x4c49464f   /* "LIFO" in ASCII */
#define OBJ_TYPE_MEM_BLOCK_ID  0x4d424c4b   /* "MBLK" in ASCII */
#define OBJ_TYPE_MBOX_ID       0x4d424f58   /* "MBOX" in ASCII */
#define OBJ_TYPE_MEM_SLAB_ID   0x534c4142   /* "SLAB" in ASCII */
#define OBJ_TYPE_MSGQ_ID       0x4d534751   /* "MSGQ" in ASCII */
#define OBJ_TYPE_MUTEX_ID      0x4d555458   /* "MUTX" in ASCII */
#define OBJ_TYPE_PIPE_ID       0x50495045   /* "PIPE" in ASCII */
#define OBJ_TYPE_SEM_ID        0x53454d34   /* "SEM4" in ASCII */
#define OBJ_TYPE_STACK_ID      0x5354434b   /* "STCK" in ASCII */
#define OBJ_TYPE_THREAD_ID     0x54485244   /* "THRD" in ASCII */
#define OBJ_TYPE_TIMER_ID      0x54494d52   /* "TIMR" in ASCII */

struct k_obj_type;
struct k_obj_core;

#ifdef CONFIG_OBJ_CORE
#define K_OBJ_CORE_INIT(_objp, _obj_type)   \
	extern struct k_obj_type _obj_type; \
	k_obj_core_init(_objp, &_obj_type)

#define K_OBJ_CORE_LINK(objp) k_obj_core_link(objp)
#else
#define K_OBJ_CORE_INIT(objp, type)   do { } while(0)
#define K_OBJ_CORE_LINK(objp, type)   do { } while(0)
#endif
/**
 * Tools may use this list as an entry point to identify all registered
 * object types and the object cores linked to them.
 */
extern sys_dlist_t obj_type_list;

struct k_obj_core_stats_desc {
	size_t  raw_size;
	size_t  query_size;

	int (*raw)(struct k_obj_core *obj_core, void *stats);
	int (*query)(struct k_obj_core *obj_core, void *stats);
	int (*reset)(struct k_obj_core *obj_core);
	int (*disable)(struct k_obj_core *obj_core);
	int (*enable)(struct k_obj_core *obj_core);
};

struct k_obj_type {
	sys_dnode_t    node;   /* Node within list of object types */
	sys_dlist_t    list;   /* List of objects of this object type */
	uint32_t       id;     /* Unique type ID */
	size_t         obj_core_offset;  /* Offset to obj_core field */
#ifdef CONFIG_OBJ_CORE_STATS
	struct k_obj_core_stats_desc *stats_desc;
#endif
};

struct k_obj_core {
	sys_dnode_t        node;   /* Object node within object type's list */
	struct k_obj_type *type;   /* Object type to which object belongs */
#ifdef CONFIG_OBJ_CORE_STATS
	void  *stats;              /* Pointer to kernel object's stats */
#endif
};

/**
 * @brief Initialize a specific object type
 *
 * Initializes a specific object type and links it into the object core
 * framework.
 *
 * @param type Pointer to the object type to initialize
 * @param id A means to identify the object type
 * @param off Offset of object core within the structure
 */
extern void z_obj_type_init(struct k_obj_type *type, uint32_t id, size_t off);

/**
 * @brief Find a specific object type by ID
 *
 * Given an object type ID, this function searches for the object type that
 * is associated with the specified type ID @a type_id.
 *
 * @param type_id  Type ID associated with object type
 *
 * @retval NULL if object type not found
 * @retval Pointer to object type if found
 */
extern struct k_obj_type *k_obj_type_find(uint32_t type_id);

/**
 * @brief Walk the object type's list of object cores
 *
 * This function walks the object type's list of object cores and invokes the
 * specified callback function on each element while locked. Although this will
 * ensure that that list is not modified, one can expect a significant penalty
 * in terms of performance and latency.
 *
 * The callback function shall either return non-zero to stop further walking,
 * or it shall return 0 to continue walking.
 *
 * @param type  Pointer to the object type
 * @param func  Callback to invoke on each object core of the object type
 * @param data  Custom data passed to the callback
 *
 * @retval non-zero if walk is terminated by the callback; otherwise 0
 */
extern int k_obj_type_walk_locked(struct k_obj_type *type,
				  int (*func)(struct k_obj_core *, void *),
				  void *data);

/**
 * @brief Walk the object type's list of object cores
 *
 * This function walks the object type's list of object cores and invokes the
 * specified callback function on each element while unlocked. This routine
 * does not prevent the addition or removal of object cores while walking the
 * list. Although this is expected to offer better responsiveness, it may
 * result in unexpected behavior. Note that locks are enabled while invoking
 * the callback.
 *
 * The callback function shall either return non-zero to stop further walking,
 * or it shall return 0 to continue walking.
 *
 * @param type  Pointer to the object type
 * @param func  Callback to invoke on each object core of the object type
 * @param data  Custom data passed to the callback
 *
 * @retval non-zero if walk is terminated by the callback; otherwise 0
 */
extern int k_obj_type_walk_unlocked(struct k_obj_type *type,
				    int (*func)(struct k_obj_core *, void *),
				    void *data);

#ifdef CONFIG_OBJ_CORE_STATS
/**
 * @brief Initialize the object type's stats descriptor
 */
static inline void k_obj_type_stats_init(struct k_obj_type *type,
					 struct k_obj_core_stats_desc *stats_desc)
{
	type->stats_desc = stats_desc;
}

/**
 * @brief Initialize the object core for statistics
 *
 * @param obj_core Pointer to the object type
 * @param stats Pointer to the object raw statistics
 */
static inline void k_obj_core_stats_init(struct k_obj_core *obj_core,
					 void *stats)
{
	obj_core->stats = stats;
}
#endif

/**
 * @brief Initialize the core of the kernel object
 *
 * Initializing the kernel object core associates it with the specified
 * kernel object type.
 *
 * @param obj_core Pointer to the kernel object to initialize
 * @param type Pointer to the kernel object type
 */
extern void k_obj_core_init(struct k_obj_core *obj_core,
			    struct k_obj_type *type);

/**
 * @brief Link the kernel object to the kernel object type list
 *
 * A kernel object can be optionally linked into the kernel object type's
 * list of objects. A kernel object must have been initialized before it
 * can be linked. Linked kernel objects can be traversed and have information
 * extracted from them by system tools.
 *
 * @param obj_core Pointer to the kernel object
 */
extern void k_obj_core_link(struct k_obj_core *obj_core);

/**
 * @brief Unlink the kernel object from the kernel object type list
 *
 * Kernel objects can be unlinked from their respective kernel object type
 * lists. If on a list, it must be done at the end of the kernel object's life
 * cycle.
 *
 * @param obj_core Pointer to the kernel object
 */
extern void k_obj_core_unlink(struct k_obj_core *obj_core);

/**
 * @brief Register kernel object for gathering statistics
 *
 * Before a kernel object can gather statistics, it must be registered to do
 * so. Registering will also automatically enable the kernel object to gather
 * its statistics.
 *
 * @param obj_core Pointer to kernel object core
 * @param stats Pointer to raw kernel statistics
 * @param stats_len Size of raw kernel statistics buffer
 *
 * @return 0 on success, -errno on failure
 */
extern int k_obj_core_stats_register(struct k_obj_core *obj_core, void *stats,
				     size_t stats_len);

/**
 * @brief Deregister kernel object from gathering statistics
 *
 * Deregistering a kernel object core from gathering statistics prevents it
 * from gathering any more statistics. It is expected to be invoked at the end
 * of a kernel object's life cycle.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @return 0 on success, -errno on failure
 */
extern int k_obj_core_stats_deregister(struct k_obj_core *obj_core);

/**
 * @brief Retrieve the raw statistics associated with the kernel object
 *
 * This function copies the raw statistics associated with the kernel object
 * core specified by @a obj_core into the buffer @a stats. Note that the size
 * of the buffer (@a stats_len) must match the size specified by the kernel
 * object type's statistics descriptor.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @return 0 on success, -errno on failure
 */
extern int k_obj_core_stats_raw(struct k_obj_core *obj_core, void *stats,
				size_t stats_len);

/**
 * @brief Retrieve the statistics associated with the kernel object
 *
 * This function copies the statistics (not raw) associated with the kernel
 * object core specified by @a obj_core into the buffer @a stats. Note that the
 * size of the buffer (@a stats_len) must match the size specified by the kernel
 * object type's statistics descriptor.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @return 0 on success, -errno on failure
 */ 
extern int k_obj_core_stats_query(struct k_obj_core *obj_core, void *stats,
				  size_t stats_len);

/**
 * @brief Reset the stats associated with the kernel object
 *
 * This function resets the statistics associated with the kernel object core
 * specified by @a obj_core.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @return 0 on success, -errno on failure
 */
extern int k_obj_core_stats_reset(struct k_obj_core *obj_core);

/**
 * @brief Stop gathering the stats associated with the kernel object
 *
 * This function temporarily stops the gathering of statistics associated with
 * the kernel object core specified by @a obj_core. The gathering of statistics
 * can be resumed by invoking :c:func :`k_obj_core_stats_enable`.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @return 0 on success, -errno on failure
 */
extern int k_obj_core_stats_disable(struct k_obj_core *obj_core);

/**
 * @brief Reset the stats associated with the kernel object
 *
 * This function resumes the gathering of statistics associated with the kernel
 * object core specified by @a obj_core.
 *
 * @param obj_core Pointer to kernel object core
 *
 * @return 0 on success, -errno on failure
 */
extern int k_obj_core_stats_enable(struct k_obj_core *obj_core);
#endif /* __KERNEL_OBJ_CORE_H__ */
