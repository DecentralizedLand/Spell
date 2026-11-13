// SPDX-License-Identifier: GPL-2.0
//
// src/sys.rs — freestanding syscall layer.
//
// Built for x86_64-unknown-none: no libc, no dynamic loader, no PLT, no GOT.
//
// 4th/5th/6th args go in r10/r8/r9 per the x86_64 SysV kernel convention,
// which is NOT the userspace SysV convention (rcx/r8/r9) — the registers
// below are the only reason this works at all.
//
// Note: "rcx" and "r11" clobbers are mandatory — syscall overwrites both
// (return RIP and RFLAGS). Omitting them corrupts the caller silently.
//
// ERRNO CONVENTION
//   kernel returns -errno in rax on failure; raw() returns that as Err(i32),
//   wrappers map it. Checking `ret > -4096` is the standard heuristic
//   (MAX_ERRNO), same as the kernel's own libc-less callers.
#![allow(dead_code)]

/// Raw 6-argument syscall. Returns raw rax: non-negative on success,
/// -errno on failure.
#[inline(always)]
pub unsafe fn raw(n: u64, a: u64, b: u64, c: u64, d: u64, e: u64, f: u64) -> i64 {
    let ret: i64;
    core::arch::asm!(
        "syscall",
        inlateout("rax") n as i64 => ret,
        in("rdi") a,
        in("rsi") b,
        in("rdx") c,
        in("r10") d,
        in("r8") e,
        in("r9") f,
        // syscall overwrites rcx (return RIP) and r11 (RFLAGS)
        lateout("rcx") _,
        lateout("r11") _,
        options(nostack)
    );
    ret
}

/// Wrap raw(): Err(-ret) on failure, Ok(ret) otherwise.
#[inline(always)]
pub fn check(ret: i64) -> Result<i64, i32> {
    if ret < 0 && ret > -4096 {
        Err(-ret as i32)
    } else {
        Ok(ret)
    }
}

// ------------------------------------------------------------------
// Wrappers needed by the CLI.
//
// open() is not used: SYS_open does not exist on all arch/config
// combinations, openat(AT_FDCWD, ...) is the portable vector.
// ------------------------------------------------------------------

pub const AT_FDCWD: i64 = -100;
pub const O_RDWR: u64 = 0o2;
pub const O_CREAT: u64 = 0o100;
pub const O_TRUNC: u64 = 0o1000;

pub fn openat(path: &str, flags: u64, mode: u64) -> Result<i32, i32> {
    // TODO: implement — check(openat(AT_FDCWD, path.as_ptr(), flags, mode))
    //   path must be NUL-terminated: callers pass b"/dev/ndump\0"
    let _ = (path, flags, mode);
    unimplemented!()
}

pub fn read(fd: i32, buf: &mut [u8]) -> Result<usize, i32> {
    // TODO: implement — check(read(fd, buf.as_mut_ptr(), buf.len()))
    let _ = (fd, buf);
    unimplemented!()
}

pub fn write(fd: i32, buf: &[u8]) -> Result<usize, i32> {
    // TODO: implement
    let _ = (fd, buf);
    unimplemented!()
}

pub fn ioctl(fd: i32, req: u64, arg: *mut u8) -> Result<i64, i32> {
    // TODO: implement — arg is *mut one of the abi::* structs
    let _ = (fd, req, arg);
    unimplemented!()
}

pub fn mmap(len: usize, prot: u32, flags: u32, fd: i32, off: i64) -> Result<usize, i32> {
    // TODO: implement — bounce buffer backing; MAP_PRIVATE|MAP_ANONYMOUS
    let _ = (len, prot, flags, fd, off);
    unimplemented!()
}

pub fn munmap(addr: usize, len: usize) -> Result<i64, i32> {
    // TODO: implement
    let _ = (addr, len);
    unimplemented!()
}

pub fn lseek(fd: i32, off: i64, whence: i32) -> Result<i64, i32> {
    // TODO: implement — ELF writer seeks back to patch e_phnum
    let _ = (fd, off, whence);
    unimplemented!()
}

pub fn close(fd: i32) -> Result<i64, i32> {
    // TODO: implement
    let _ = fd;
    unimplemented!()
}

pub fn exit_group(code: i32) -> ! {
    unsafe { raw(231, code as u64, 0, 0, 0, 0, 0) };
    // unreachable: SYS_exit_group never returns
    loop {}
}

pub fn getpid() -> u32 {
    // TODO: implement — used for --pid self-aliasing in tools/selfmap
    unimplemented!()
}

// Minimal formatting over write(1, ...): no alloc, no std::fmt.
pub fn puts(s: &str) {
    let _ = write(1, s.as_bytes());
}

pub fn put_u64(mut v: u64) {
    let mut buf = [0u8; 20];
    let mut i = buf.len();
    if v == 0 {
        puts("0");
        return;
    }
    while v > 0 {
        i -= 1;
        buf[i] = b'0' + (v % 10) as u8;
        v /= 10;
    }
    let _ = write(1, &buf[i..]);
}
