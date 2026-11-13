/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ndump_ring.c — single-producer / single-consumer kfifo page ring.
 *
 * MODEL
 *   producer : the dump work item (ndump_vma_dump -> ndump_read_frame)
 *   consumer : read(2) on /dev/ndump
 *   fifo     : kfifo of NDUMP_SLOT_COUNT * NDUMP_SLOT_SIZE bytes
 *   locks    : one mutex protects the kfifo; two wait queues provide
 *              producer/consumer backpressure separately
 *
 * BACKPRESSURE
 *   producer: wait_event_interruptible(space_wait, kfifo_avail >= need)
 *   consumer: wait_event_interruptible(fifo_wait,  kfifo_len  >  0)
 *   Both wake the opposite queue after each transfer. Bounded kernel memory
 *   is the point: a 40 GiB target uses 1 MiB of module RAM.
 *
 * ATOMICITY
 *   A record is pushed whole. read() returns whole records only; if the user
 *   buffer is too small for the next record, return the bytes accumulated so
 *   far (0 if none, so the caller can detect a too-small buffer and grow).
 *   Never split a record across two read() calls.
 *
 * LOCK ORDER (avoid deadlock)
 *   mmap_read_lock  ->  walk/copy  ->  unlock  ->  fifo_lock  ->  wake
 *   Never take mmap_read_lock while holding fifo_lock.
 *
 * TODO: implement.
 */
