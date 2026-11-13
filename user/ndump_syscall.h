/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ndump_syscall.h — freestanding syscall layer.
 *
 * Built with -ffreestanding -nostdlib -static -fno-stack-protector.
 * There is no libc, no dynamic loader, no PLT, no GOT in the output.
 *
 * All numbers come from <asm/unistd_64.h> at build time — never hardcoded —
 * so a kernel ABI change fails the build instead of silently calling the
 * wrong vector.
 *
 * 4th/5th/6th args go in r10/r8/r9 per the x86_64 SysV kernel convention,
 * which is NOT the userspace SysV convention (rcx/r8/r9) — the registers
 * below are the only reason this works at all.
 *
 *   static inline long nd_syscall6(long n, long a, long b, long c,
 *                                  long d, long e, long f)
 *   {
 *           register long r10 __asm__("r10") = d;
 *           register long r8  __asm__("r8")  = e;
 *           register long r9  __asm__("r9")  = f;
 *           long ret;
 *           __asm__ volatile ("syscall"
 *                             : "=a"(ret)
 *                             : "a"(n), "D"(a), "S"(b), "d"(c),
 *                               "r"(r10), "r"(r8), "r"(r9)
 *                             : "rcx", "r11", "memory");
 *           return ret;
 *   }
 *
 * Note: "rcx" and "r11" clobbers are mandatory — syscall overwrites both
 * (return RIP and RFLAGS). Omitting them corrupts the caller silently.
 *
 * WRAPPERS NEEDED
 *   nd_openat, nd_read, nd_write, nd_ioctl, nd_mmap, nd_munmap,
 *   nd_close, nd_exit_group, nd_getpid
 *
 * Raw open() is not used: SYS_open does not exist on all arch/config
 * combinations, openat(AT_FDCWD, ...) is the portable vector.
 *
 * TODO: implement.
 */
