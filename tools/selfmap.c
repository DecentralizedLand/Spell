/* SPDX-License-Identifier: GPL-2.0 */
/*
 * tools/selfmap.c — differential-test target for nimbledump.
 *
 * PURPOSE
 *   Produce an independent, trustworthy record of what a process's memory
 *   contains, WITHOUT using /proc/pid/mem, ptrace or process_vm_readv, so
 *   the validation does not rely on the same interface the tool refuses to
 *   use.
 *
 * HOW
 *   1. mmap a set of regions with known deterministic content
 *      (seq fill, pattern fill, 8 KiB THP-aligned block, a read-only page).
 *   2. write each region's geometry + a hash of its content to a sidecar
 *      file, and print the VMA ranges it created.
 *   3. sleep so nimbledump can attach.
 *
 * Then tools/validate.sh runs `ndump -p <pid>` and diffs:
 *   - region geometry   vs the module's --vma-list
 *   - region content    vs the hashes in the sidecar
 *
 * Any byte-level mismatch in the anonymous MAP_PRIVATE regions is a page
 * walk bug (wrong level, THP interior page, or offset_in_page slip).
 */
