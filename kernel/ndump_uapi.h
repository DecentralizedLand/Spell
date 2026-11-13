/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nimbledump — shared kernel <-> user ABI.
 *
 * This header is the entire contract between nimbledump.ko and the ndump CLI.
 * It must stay valid C for both the kernel build and the freestanding
 * (-nostdlib) user build, so it uses only kernel-style fixed-width types and
 * no libc headers.
 */
#ifndef NDUMP_UAPI_H
#define NDUMP_UAPI_H

#define NDUMP_ABI_VERSION	1
#define NDUMP_IOC_MAGIC		'Z'

/* ------------------------------------------------------------------ */
/* ATTACH: validate pid, pin mm, capture identity for PID-reuse checks */
/* ------------------------------------------------------------------ */
struct ndump_attach {
	__u32	pid;
	__u32	flags;		/* NDUMP_ATTACH_* */
	__u64	cookie;		/* out: opaque handle for this session */
};

#define NDUMP_ATTACH_ANY_NS	(1u << 0)	/* match pid in any pidns */

/* ------------------------------------------------------------------ */
/* VMA_LIST: iterative address-space map, no data copied               */
/* ------------------------------------------------------------------ */
struct ndump_vma_info {
	__u64	start;
	__u64	end;
	__u64	flags;		/* vm_flags as reported by the kernel */
	__u32	index;
	__u32	_next;		/* 0 = last entry */
};

/* ------------------------------------------------------------------ */
/* Region filter: repeatable, intersected with the VMA list            */
/* ------------------------------------------------------------------ */
struct ndump_region {
	__u64	start;
	__u64	end;
	__u32	flags;		/* NDUMP_REGION_* */
	__u32	_pad;
};

#define NDUMP_REGION_ALL	0x0000u		/* whole address space */
#define NDUMP_REGION_PRESENT	0x0001u		/* skip pte_none holes */

/* ------------------------------------------------------------------ */
/* Statistics                                                          */
/* ------------------------------------------------------------------ */
struct ndump_stat {
	__u64	pages_ok;	/* frames copied from a pte            */
	__u64	pages_zero;	/* holes / zero-pfn, emitted as zeros  */
	__u64	pages_skip;	/* VM_IO / PFNMAP / spec / not present */
	__u64	bytes_out;	/* payload bytes handed to user space  */
	__u64	faults;		/* copy_from_kernel_nofault failures   */
	__u32	vma_count;
	__u32	status;		/* NDUMP_STAT_*                        */
};

#define NDUMP_STAT_RUNNING	0u
#define NDUMP_STAT_DONE		1u
#define NDUMP_STAT_PARTIAL	2u	/* target mm changed under us */
#define NDUMP_STAT_ABORTED	3u

/* ------------------------------------------------------------------ */
/* Stream framing returned by read(2)                                  */
/*                                                                     */
/* read() only ever returns whole records. Layout:                     */
/*   struct ndump_rec  (16 bytes)                                      */
/*   payload[len]      when flags has NDUMP_F_DATA                     */
/* Records are back-to-back; a short read at a record boundary is      */
/* legal and the consumer must loop.                                   */
/* ------------------------------------------------------------------ */
struct ndump_rec {
	__u64	vaddr;		/* guest virtual address of first byte  */
	__u32	len;		/* payload length (0 for ZERO / SKIP)   */
	__u32	flags;
};

#define NDUMP_F_DATA	0x00000001u	/* payload follows                */
#define NDUMP_F_ZERO	0x00000002u	/* hole; consumer fills with 0    */
#define NDUMP_F_SKIP	0x00000004u	/* unmappable; metadata only      */

/* ------------------------------------------------------------------ */
/* ioctl numbers                                                       */
/* ------------------------------------------------------------------ */
#define NDUMP_IOC_ATTACH	_IOWR(NDUMP_IOC_MAGIC, 1, struct ndump_attach)
#define NDUMP_IOC_DETACH	_IO  (NDUMP_IOC_MAGIC, 2)
#define NDUMP_IOC_VMA_LIST	_IOR (NDUMP_IOC_MAGIC, 3, struct ndump_vma_info)
#define NDUMP_IOC_ADD_REGION	_IOW (NDUMP_IOC_MAGIC, 4, struct ndump_region)
#define NDUMP_IOC_START		_IO  (NDUMP_IOC_MAGIC, 5)
#define NDUMP_IOC_STOP		_IO  (NDUMP_IOC_MAGIC, 6)
#define NDUMP_IOC_STAT		_IOR (NDUMP_IOC_MAGIC, 7, struct ndump_stat)

#define NDUMP_DEVICE_NAME	"ndump"

#endif /* NDUMP_UAPI_H */
