/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ndump_vma.c — address-space reconstruction from vm_area_struct.
 *
 *   mm = get_task_mm(task);           // pins mm_users, survives task exit
 *   if (!mm) return -ESRCH;           // kernel thread: nothing to dump
 *
 *   mmap_read_lock(mm);
 *   VMA_ITERATOR(vmi, mm, 0);
 *   for_each_vma(vmi, vma) {
 *           // report vm_start, vm_end, vm_flags
 *           // skip data copy when vm_flags & (VM_IO | VM_PFNMAP | VM_DONTDUMP)
 *   }
 *   mmap_read_unlock(mm);
 *
 * NOTES
 *   - for_each_vma() walks the maple tree and skips holes; do not hand-roll
 *     a linked-list walk, `vm_next` ordering is not guaranteed under
 *     concurrent modification.
 *   - The read lock is released between VMAs. A long dump must not hold
 *     mmap_read_lock for its entire duration or the target's page faults
 *     stall.
 *   - VMA count is capped at NDUMP_MAX_VMA_ENTRIES; a pathological process
 *     gets NDUMP_STAT_PARTIAL rather than an unbounded kernel allocation.
 *   - Return -ENOMEM if kmalloc_array for the region intersection fails.
 *
 * TODO: implement.
 */
