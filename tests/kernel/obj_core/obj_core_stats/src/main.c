#include <zephyr/ztest.h>
#include <zephyr/sys/mem_blocks.h>

SYS_MEM_BLOCKS_DEFINE(mem_block, 32, 4, 16);

K_MEM_SLAB_DEFINE(mem_slab, 32, 4, 16);

K_SEM_DEFINE(sem, 0, 1);
K_SEM_DEFINE(wake, 0, 1);

static void thread_entry(void *, void *, void *);
K_THREAD_DEFINE(thread, 512, thread_entry, NULL, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, 0);

static void thread_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem1, K_FOREVER);

	k_busy_wait(1000);

	k_sem_give(&wake);
	k_sem_take(&sem1, K_FOREVER);
}

ZTEST(obj_core, test_obj_core_stats_thread)
{
	k_thread_runtime_stats_t  runtime_stats;
	k_thread_runtime_stats_t  obj_core_stats;

	k_thread_runtime_stats_get(&thread, &runtime_stats);
	k_obj_core_stats_query(K_OBJ_CORE(&thread), &obj_core_stats,
			       sizeof(k_thread_runtime_stats_t));
}

ZTEST(obj_core, test_obj_core_system)
{
	int  i;
	char cpu_str[16];

	/*
	 * Use the existing object cores in the _cpu and z_kerenl structures
	 * as we should not be creating new ones.
	 */

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		sprintf(cpu_str, "CPU%d", i);
		common_obj_core_test(OBJ_TYPE_CPU_ID, cpu_str,
				     K_OBJ_CORE(&_kernel.cpus[i]), NULL);
	}

	common_obj_core_test(OBJ_TYPE_KERNEL_ID, "_kernel",
			     K_OBJ_CORE(&_kernel), NULL);
}

ZTEST(obj_core, test_obj_core_mem_slab)
{
	k_mem_slab_init(&slab2, slab2_buffer, 32, 8);
	common_obj_core_test(OBJ_TYPE_MEM_SLAB_ID, "memory slab",
			     K_OBJ_CORE(&slab1), K_OBJ_CORE(&slab2));
}

ZTEST_SUITE(obj_core, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
