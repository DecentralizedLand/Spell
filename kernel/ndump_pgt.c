/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ndump_pgt.c — page-table walk: pgd -> p4d -> pud -> pmd -> pte.
 *
 * CONTROL FLOW (see DESIGN.md section 5 for the annotated listing)
 *
 *   pgd_index/pgd_offset
 *     pgd_none || pgd_bad          -> hole
 *   p4d_offset                    (folds to pgd on 4-level, no manual index)
 *   pud_offset
 *     pud_none                     -> hole
 *     pud_leaf                     -> 1 GiB page (CONFIG_HUGETLBFS)
 *   pmd_offset
 *     pmd_none                     -> hole
 *     pmd_leaf || pmd_trans_huge   -> 2 MiB THP; handle all of it at once
 *   pte_offset_map_lock
 *     pte_present                  -> frame
 *     pte_special                  -> skip (no struct page)
 *     is_zero_pfn(pte_pfn)         -> emit NDUMP_F_ZERO, do not map
 *   pte_unmap_unlock
 *
 * NON-OBVIOUS PARTS
 *   - Always use p4d_offset(); never index mm->pgd directly, or the module
 *     silently breaks on CONFIG_PGTABLE_LEVELS=5.
 *   - pgd_bad()/pud_bad() checks matter; a torn-down page table without them
 *     is a host crash inside a kernel module, not a bad dump.
 *   - THP: copy the full PMD range via pmd_page() and slice per-address on
 *     the way out, so tail pages of a compound page are lifted correctly.
 *   - kmap_local_page()/kunmap_local() around the copy; requires a sleepable
 *     context (work item, not softirq) under CONFIG_PREEMPT_RT.
 *   - Copy with copy_from_kernel_nofault() when the frame is untrusted
 *     (KASAN, or page_type looks implausible). Count into stat.faults and
 *     emit NDUMP_F_SKIP instead of faulting the host.
 *
 * TODO: implement.
 */
