// SPDX-License-Identifier: GPL-2.0
//
// src/abi.rs — user-space mirror of kernel/ndump_uapi.h.
//
// The C header is the single source of truth for layout; these repr(C)
// structs must match it byte-for-byte, and `NDUMP_ABI_VERSION` is checked
// against the module at ATTACH time (mismatch -> -EPROTO). If the header
// changes, change this file in the same commit — there is no bindgen step,
// because freestanding + no-std bindgen would drag in a std dependency
// for a header that is 100 lines of plain structs.
#![allow(dead_code)]

pub const NDUMP_ABI_VERSION: u32 = 1;
pub const NDUMP_IOC_MAGIC: u8 = b'Z';
pub const NDUMP_DEVICE_NAME: &str = "/dev/ndump";

// ioctl encoding — _IO* family computed by hand, no linux/ioctl.h.
const IOC_NONE: u64 = 0;
const IOC_WRITE: u64 = 1;
const IOC_READ: u64 = 2;

const fn ioc(dir: u64, ty: u8, nr: u8, size: usize) -> u64 {
    (dir << 30) | ((size as u64) << 16) | ((ty as u64) << 8) | nr as u64
}
const fn iowr<T>(ty: u8, nr: u8) -> u64 {
    ioc(IOC_READ | IOC_WRITE, ty, nr, core::mem::size_of::<T>())
}
const fn iow<T>(ty: u8, nr: u8) -> u64 {
    ioc(IOC_WRITE, ty, nr, core::mem::size_of::<T>())
}
const fn ior<T>(ty: u8, nr: u8) -> u64 {
    ioc(IOC_READ, ty, nr, core::mem::size_of::<T>())
}
const fn io(ty: u8, nr: u8) -> u64 {
    ioc(IOC_NONE, ty, nr, 0)
}

// ------------------------------------------------------------------
// ATTACH: validate pid, pin mm, capture identity for PID-reuse checks
// ------------------------------------------------------------------
#[repr(C)]
pub struct NdumpAttach {
    pub pid: u32,
    pub flags: u32,  // NDUMP_ATTACH_*
    pub cookie: u64, // out: opaque handle for this session
}

pub const NDUMP_ATTACH_ANY_NS: u32 = 1 << 0;

// ------------------------------------------------------------------
// VMA_LIST: iterative address-space map, no data copied
// ------------------------------------------------------------------
#[repr(C)]
pub struct NdumpVmaInfo {
    pub start: u64,
    pub end: u64,
    pub flags: u64, // vm_flags as reported by the kernel
    pub index: u32,
    pub next: u32, // 0 = last entry (C name: _next)
}

// ------------------------------------------------------------------
// Region filter: repeatable, intersected with the VMA list
// ------------------------------------------------------------------
#[repr(C)]
pub struct NdumpRegion {
    pub start: u64,
    pub end: u64,
    pub flags: u32, // NDUMP_REGION_*
    pub pad: u32,
}

pub const NDUMP_REGION_ALL: u32 = 0x0000; // whole address space
pub const NDUMP_REGION_PRESENT: u32 = 0x0001; // skip pte_none holes

// ------------------------------------------------------------------
// Statistics
// ------------------------------------------------------------------
#[repr(C)]
#[derive(Default)]
pub struct NdumpStat {
    pub pages_ok: u64,   // frames copied from a pte
    pub pages_zero: u64, // holes / zero-pfn, emitted as zeros
    pub pages_skip: u64, // VM_IO / PFNMAP / spec / not present
    pub bytes_out: u64,  // payload bytes handed to user space
    pub faults: u64,     // copy_from_kernel_nofault failures
    pub vma_count: u32,
    pub status: u32, // NDUMP_STAT_*
}

pub const NDUMP_STAT_RUNNING: u32 = 0;
pub const NDUMP_STAT_DONE: u32 = 1;
pub const NDUMP_STAT_PARTIAL: u32 = 2; // target mm changed under us
pub const NDUMP_STAT_ABORTED: u32 = 3;

// ------------------------------------------------------------------
// Stream framing returned by read(2)
//
// read() only ever returns whole records. Layout:
//   NdumpRec  (16 bytes)
//   payload[len]      when flags has NDUMP_F_DATA
// Records are back-to-back; a short read at a record boundary is
// legal and the consumer must loop.
// ------------------------------------------------------------------
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NdumpRec {
    pub vaddr: u64, // guest virtual address of first byte
    pub len: u32,   // payload length (0 for ZERO / SKIP)
    pub flags: u32,
}

pub const NDUMP_F_DATA: u32 = 0x0000_0001; // payload follows
pub const NDUMP_F_ZERO: u32 = 0x0000_0002; // hole; consumer fills with 0
pub const NDUMP_F_SKIP: u32 = 0x0000_0004; // unmappable; metadata only

// ------------------------------------------------------------------
// ioctl numbers
// ------------------------------------------------------------------
pub const NDUMP_IOC_ATTACH: u64 = iowr::<NdumpAttach>(NDUMP_IOC_MAGIC, 1);
pub const NDUMP_IOC_DETACH: u64 = io(NDUMP_IOC_MAGIC, 2);
pub const NDUMP_IOC_VMA_LIST: u64 = ior::<NdumpVmaInfo>(NDUMP_IOC_MAGIC, 3);
pub const NDUMP_IOC_ADD_REGION: u64 = iow::<NdumpRegion>(NDUMP_IOC_MAGIC, 4);
pub const NDUMP_IOC_START: u64 = io(NDUMP_IOC_MAGIC, 5);
pub const NDUMP_IOC_STOP: u64 = io(NDUMP_IOC_MAGIC, 6);
pub const NDUMP_IOC_STAT: u64 = ior::<NdumpStat>(NDUMP_IOC_MAGIC, 7);

// Size sanity: the framing header must be exactly 16 bytes or the
// record parser desyncs against the kernel's writer.
const _: () = assert!(core::mem::size_of::<NdumpRec>() == 16);
