/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ndump_task.c — pid -> task_struct, and PID-reuse validation.
 *
 * TWO STRATEGIES (select at build time, see DESIGN.md section 3)
 *
 *   NDUMP_TASK_WALK_RAW=n
 *       find_get_task_by_vpid(pid): exported, pidns-correct.
 *
 *   NDUMP_TASK_WALK_RAW=y
 *       manual walk of the global task list:
 *
 *         rcu_read_lock();
 *         for (t = &init_task; ; t = list_entry(t->tasks.next, ...)) {
 *                 if (t->pid == pid) { get_task_struct(t); break; }
 *                 if (t->tasks.next == &init_task.tasks) { t = NULL; break; }
 *         }
 *         rcu_read_unlock();
 *
 *       Uses only `init_task` (exported) plus compile-time struct offsets
 *       from <linux/sched.h>. No hardcoded offsetof() numbers, so this does
 *       not break when the kernel ABI shifts.
 *
 * IDENTITY GUARD (mandatory, see DESIGN.md section 10)
 *   capture task->start_time + task->mm at ATTACH, re-check before every
 *   region. A recycled pid would otherwise dump the wrong process's memory.
 *
 * CROSS-NAMESPACE
 *   flags & NDUMP_ATTACH_ANY_NS -> compare task_pid_nr_ns(t, target_ns)
 *   instead of t->pid. target_ns is resolved once at ATTACH.
 *
 * TODO: implement.
 */
