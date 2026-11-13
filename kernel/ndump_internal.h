/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nimbledump — private module state and cross-file declarations.
 */
#ifndef NDUMP_INTERNAL_H
#define NDUMP_INTERNAL_H

#include <linux/types.h>
#include <linux/kfifo.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/mm_types.h>

#include "ndump_uapi.h"

#define NDUMP_SLOT_SIZE		PAGE_SIZE
#define NDUMP_SLOT_COUNT	256	/* 1 MiB of in-flight data max */
#define NDUMP_MAX_REGIONS	64
#define NDUMP_MAX_VMA_ENTRIES	65536

/* One attached target. One instance per open() of /dev/ndump. */
struct ndump_session {
	struct miscdevice	dev;
	struct task_struct	*task;		/* refcounted, may be NULL */
	struct mm_struct	*mm;		/* pinned via get_task_mm() */
	u64			start_time;	/* PID-reuse guard */
	u64			magic;

	struct ndump_region	regions[NDUMP_MAX_REGIONS];
	u32			nr_regions;

	struct kfifo		fifo;		/* NDUMP_SLOT_SIZE slots */
	struct mutex		fifo_lock;
	wait_queue_head_t	fifo_wait;	/* consumer side */
	wait_queue_head_t	space_wait;	/* producer side */

	struct work_struct	producer;
	struct ndump_stat	stat;
	u32			state;		/* S_IDLE .. S_DONE */
	bool			stop;
};

/* --- ndump_task.c ------------------------------------------------- */
/* Resolve pid -> task_struct. Returns a refcounted task or ERR_PTR.  */
struct task_struct *ndump_find_task(u32 pid, bool any_ns);
/* True if the task still matches the session's captured identity.   */
bool ndump_task_still_valid(struct ndump_session *s);

/* --- ndump_vma.c -------------------------------------------------- */
/* Enumerate VMAs into the fifo as SKIP records, or into @out for    */
/* the VMA_LIST ioctl (user pointer, may fault).                    */
int ndump_vma_list(struct ndump_session *s, struct ndump_vma_info __user *out);
/* Walk VMA data ranges and push records to the fifo.               */
int ndump_vma_dump(struct ndump_session *s);

/* --- ndump_pgt.c -------------------------------------------------- */
/* Walk one virtual address, return the page or NULL.                */
struct page *ndump_walk_va(struct mm_struct *mm, unsigned long addr,
			   unsigned long *offset, bool *zero_pfn);
/* Push one VMA data record; owns the kmap/copy/kunmap cycle.       */
int ndump_copy_range(struct ndump_session *s, unsigned long start,
		     unsigned long end);

/* --- ndump_read.c ------------------------------------------------- */
/* pte -> page -> kmap_local_page -> fifo. Returns bytes queued.     */
int ndump_read_frame(struct ndump_session *s, struct page *page,
		     unsigned long offset, unsigned long len,
		     unsigned long vaddr);

/* --- ndump_ring.c ------------------------------------------------- */
int  ndump_ring_init(struct ndump_session *s);
void ndump_ring_free(struct ndump_session *s);
/* Producer: block until space, push one complete record.            */
int  ndump_ring_push(struct ndump_session *s, const struct ndump_rec *rec,
		     const void *payload, unsigned int len);
/* Consumer: copy at most @len bytes of complete records to user.    */
ssize_t ndump_ring_pop(struct ndump_session *s, void __user *ubuf,
		       size_t len);
void ndump_ring_wake_all(struct ndump_session *s);

#endif /* NDUMP_INTERNAL_H */
