// SPDX-License-Identifier: GPL-2.0
//
// src/main.rs — CLI entry point and orchestration.
//
// FLOW
//   1. parse argv                 (parse_args)
//   2. open /dev/ndump            (dev::open)
//   3. ioctl ATTACH { pid }       -> cookie, module pins the mm
//   4. --vma-list ? dump map and exit
//   5. ADD_REGION for each -r;  none given -> whole address space
//   6. elf::begin (or raw file header)
//   7. START, then loop read():
//        - parse ndump_rec frames
//        - DATA -> write payload, record PT_LOAD
//        - ZERO -> write zeroes (or seek, in --raw sparse mode)
//        - SKIP -> record hole only
//   8. STAT, DETACH, elf::finish, close
//
// NO-LIBC DETAILS
//   - target x86_64-unknown-none: no std, no runtime, no unwind tables.
//   - entry is a real `_start`; argc/argv come off the stack in asm,
//     since there is no crt0 to do it.
//   - no println: a minimal itoa/hex writer over syscall write(1, ...).
//   - exit via sys::exit_group(); returning from _start is undefined.
//   - a static .bss + a fixed 1 MiB bounce buffer, no allocator.
#![no_std]
#![no_main]

mod abi;
mod dev;
mod elf;
mod scan;
mod sys;

use core::arch::global_asm;
use core::panic::PanicInfo;

global_asm!(
    ".global _start",
    "_start:",
    "  mov rbp, rsp",
    "  mov rdi, [rsp]",        // argc
    "  lea rsi, [rsp + 8]",    // argv
    "  call rust_main",
    "  mov rdi, rax",
    "  mov eax, 231",          // SYS_exit_group
    "  syscall",
);

/// Session state threaded through the whole flow — no globals, no heap.
pub struct Session {
    pub fd: i32,
    pub pid: u32,
    pub cookie: u64,
    pub out_fd: i32,
    pub raw: bool,
    pub vma_list_only: bool,
    pub verbose: bool,
    pub regions: [(u64, u64); 64],
    pub n_regions: usize,
    pub stat: abi::NdumpStat,
}

/// Return value convention: Ok(0) or negative errno, like the C version.
pub type NdResult<T> = Result<T, i32>;

#[no_mangle]
pub extern "C" fn rust_main(argc: isize, argv: *const *const u8) -> isize {
    // TODO: implement.
    //   1. parse_args(argc, argv, &mut s)
    //   2. dev::open(&mut s)
    //   3. dev::attach(&mut s)
    //   4. if s.vma_list_only { dev::vma_list(&s, cb); return 0 }
    //   5. for each region: dev::add_region(&mut s, start, end)
    //   6. elf::begin(&mut s)
    //   7. dev::start(&s); drain loop over sys::read parsing abi::NdumpRec
    //   8. dev::stat(&s); dev::detach(&s); elf::finish(&mut s); sys::close
    let _ = (argc, argv);
    unimplemented!()
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // No unwinding under panic=abort; a bare exit is all the contract needs.
    sys::exit_group(127)
}
