/* SPDX-License-Identifier: GPL-2.0 */
/*
 * user/ndump_dev.c — client for /dev/ndump.
 *
 *   fd = nd_openat(AT_FDCWD, "/dev/ndump", O_RDWR, 0);
 *   ioctl(fd, NDUMP_IOC_ATTACH, &attach);
 *   ioctl(fd, NDUMP_IOC_ADD_REGION, &region);
 *   ioctl(fd, NDUMP_IOC_START, 0);
 *   while ((n = read(fd, buf, sizeof buf)) > 0) { ... }
 *   ioctl(fd, NDUMP_IOC_STAT, &stat);
 *   close(fd);
 *
 * ERROR MAPPING
 *   -ENODEV  -> module not loaded
 *   -EACCES  -> not root / capabilities (CAP_SYS_ADMIN not granted)
 *   -ESRCH   -> pid does not resolve to a user process (kernel thread or gone)
 *   -EBUSY   -> another session already attached to that mm
 *   -EPROTO  -> ABI version mismatch, rebuild user side against the module
 *
 * The cookie returned by ATTACH is echoed into every subsequent ioctl's
 * unused padding field and checked by the module, so a stale fd cannot
 * operate on a reused session slot.
 *
 * TODO: implement.
 */
