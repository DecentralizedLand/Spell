/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ndump_read.c — frame -> kernel buffer -> ring.
 *
 *   int ndump_read_frame(session, page, offset, len, vaddr)
 *   {
 *           void *kaddr = kmap_local_page(page);
 *           memcpy(slot, kaddr + offset, len);
 *           kunmap_local(kaddr);
 *           stat.pages_ok++;
 *           return ndump_ring_push(session, &rec, slot, len);
 *   }
 *
 * CONSTRAINTS
 *   - len is clamped to NDUMP_SLOT_SIZE; a record never spans two slots.
 *   - offset is always < PAGE_SIZE and derived from offset_in_page(), never
 *     from a user-supplied value.
 *   - The fifo push happens AFTER kunmap_local(): holding a kmap across a
 *     potential sleep in the ring's backpressure path is a bug on 32-bit.
 *   - Prefer the zero-copy kfifo_in_locked variant with a scratch slot only
 *     if the fifo has contiguous space; otherwise bounce through
 *     NDUMP_SLOT_SIZE of kmalloc memory owned by the session.
 *
 * TODO: implement.
 */
