// SPDX-License-Identifier: GPL-2.0
//
// src/dev.rs — client for /dev/ndump.
//
//   fd = sys::openat("/dev/ndump", O_RDWR, 0);
//   ioctl(fd, NDUMP_IOC_ATTACH, &attach);
//   ioctl(fd, NDUMP_IOC_ADD_REGION, &region);
//   ioctl(fd, NDUMP_IOC_START, 0);
//   while ((n = sys::read(fd, buf)) > 0) { ... }
//   ioctl(fd, NDUMP_IOC_STAT, &stat);
//   sys::close(fd);
//
// ERROR MAPPING
//   -ENODEV  -> module not loaded
//   -EACCES  -> not root / capabilities (CAP_SYS_ADMIN not granted)
//   -ESRCH   -> pid does not resolve to a user process (kernel thread or gone)
//   -EBUSY   -> another session already attached to that mm
//   -EPROTO  -> ABI version mismatch, rebuild user side against the module
//
// The cookie returned by ATTACH is echoed into every subsequent ioctl's
// unused padding field and checked by the module, so a stale fd cannot
// operate on a reused session slot.
#![allow(dead_code)]

use crate::abi;
use crate::sys;
use crate::{NdResult, Session};

pub fn open(s: &mut Session) -> NdResult<()> {
    // TODO: implement — openat(NDUMP_DEVICE_NAME, O_RDWR, 0);
    //   -ENODEV  -> print "module not loaded" and bail
    let _ = s;
    unimplemented!()
}

pub fn attach(s: &mut Session) -> NdResult<()> {
    // TODO: implement — NdumpAttach { pid, flags: 0, cookie: 0 };
    //   ioctl(ATTACH); store returned cookie in s.cookie;
    //   verify abi::NDUMP_ABI_VERSION first -> -EPROTO on mismatch
    let _ = s;
    unimplemented!()
}

pub fn detach(s: &Session) -> NdResult<()> {
    // TODO: implement — ioctl(DETACH) with cookie in padding
    let _ = s;
    unimplemented!()
}

pub fn add_region(s: &mut Session, start: u64, end: u64) -> NdResult<()> {
    // TODO: implement — bounds-check s.regions (cap 64), then
    //   ioctl(ADD_REGION, NdumpRegion { start, end, flags: ALL, pad: 0 })
    let _ = (s, start, end);
    unimplemented!()
}

pub fn start(s: &Session) -> NdResult<()> {
    // TODO: implement — ioctl(START)
    let _ = s;
    unimplemented!()
}

pub fn stop(s: &Session) -> NdResult<()> {
    // TODO: implement — ioctl(STOP); also called on SIGINT-style abort
    let _ = s;
    unimplemented!()
}

pub fn stat(s: &mut Session) -> NdResult<()> {
    // TODO: implement — ioctl(STAT) fills s.stat
    let _ = s;
    unimplemented!()
}

/// VMA_LIST is a stream of fixed records terminated by next == 0,
/// same back-to-back rule as the data path: a short read at a record
/// boundary is legal, the loop must reassemble.
pub fn vma_list(s: &Session, mut cb: impl FnMut(&abi::NdumpVmaInfo)) -> NdResult<()> {
    // TODO: implement — loop ioctl(VMA_LIST) into a 4 KiB buffer,
    //   slice into NdumpVmaInfo, skip the zeroed terminator, call cb
    let _ = (s, &mut cb);
    unimplemented!()
}
