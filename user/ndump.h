/* SPDX-License-Identifier: GPL-2.0 */
/*
 * user/ndump.h — shared user-space declarations.
 *
 * Re-includes the kernel ABI header verbatim (same file, one source of
 * truth). The kernel header uses __u64/__u32 and _IO* macros from
 * <linux/ioctl.h>; under -nostdlib those come from the kernel UAPI tree,
 * not from libc, so no libc include is needed.
 */
#ifndef NDUMP_H
#define NDUMP_H

#include "../kernel/ndump_uapi.h"
#include "../kernel/ndump_internal.h"	/* for NDUMP_SLOT_SIZE only */

#define ND_OK		0
#define ND_ERR_IO	-1
#define ND_ERR_ABI	-2
#define ND_ERR_PERM	-3

struct nd_session {
	int	fd;
	__u32	pid;
	__u64	cookie;
	int	out_fd;
	int	raw;
	int	vma_list_only;
	int	verbose;
	__u64	regions[64][2];
	int	n_regions;
	struct ndump_stat stat;
};

/* main.c */
int  nd_parse_args(int argc, char **argv, struct nd_session *s);

/* ndump_dev.c — device client */
int  nd_open(struct nd_session *s);
int  nd_attach(struct nd_session *s);
int  nd_add_region(struct nd_session *s, __u64 start, __u64 end);
int  nd_start(struct nd_session *s);
int  nd_stop(struct nd_session *s);
int  nd_stat(struct nd_session *s);
int  nd_vma_list(struct nd_session *s, void (*cb)(const struct ndump_vma_info *));

/* ndump_elf.c — ET_CORE writer */
int  nd_elf_begin(struct nd_session *s);
int  nd_elf_add_segment(struct nd_session *s, __u64 vaddr, __u64 len,
			__u64 file_off);
int  nd_elf_finish(struct nd_session *s);

/* ndump_scan.c — post-processing */
int  nd_scan_pattern(const void *buf, __u64 len, const char *needle);

#endif /* NDUMP_H */
