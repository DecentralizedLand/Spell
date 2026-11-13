/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ndump_main.c — module lifecycle, misc device, fops, ioctl dispatch.
 *
 * RESPONSIBILITY
 *   - register /dev/ndump via misc_register()
 *   - one struct ndump_session per open(); freed in .release
 *   - ioctl dispatch: ATTACH / DETACH / VMA_LIST / ADD_REGION /
 *     START / STOP / STAT
 *   - validate every user pointer with copy_from_user / put_user
 *   - bind .read to ndump_ring_pop, .release to full teardown
 *
 * ORDERING RULES
 *   - ATTACH must succeed before START; START before read() yields data
 *   - STOP is synchronous: cancel_work_sync() the producer before tearing
 *     down the fifo, otherwise the walker writes into freed memory
 *   - module_exit() must survive sessions that are still open (misc_deregister
 *     does not close fds; refuse unload with refcount > 0)
 *
 * SEE docs/DESIGN.md section 7 (ioctl ABI) and 10 (failure modes).
 *
 * TODO: implement.
 */
