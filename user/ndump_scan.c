/* SPDX-License-Identifier: GPL-2.0 */
/*
 * user/ndump_scan.c — cheap post-processing over the drained stream.
 *
 *   nd_scan_pattern(buf, len, needle)
 *       naive two-byte skip search (Boyer-Moore-Horspool). Reports offsets;
 *       used by --scan to locate keys/strings without a full dump-to-disk
 *       pass. Deliberately dumb: a real matcher would need the whole address
 *       space resident, which defeats the streaming design.
 *
 *   nd_scan_entropy(buf, len)
 *       Shannon entropy per 1 KiB block; flags packed/encrypted regions.
 *       Useful for triaging which VMA is worth a targeted second pass.
 *
 * Both functions operate on the caller's bounce buffer only and never
 * retain state between records, so a 40 GiB target is scannable in constant
 * memory.
 *
 * TODO: implement.
 */
