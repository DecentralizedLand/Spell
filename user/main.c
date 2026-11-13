/* SPDX-License-Identifier: GPL-2.0 */
/*
 * user/main.c — CLI entry point and orchestration.
 *
 * FLOW
 *   1. parse argv                 (nd_parse_args)
 *   2. open /dev/ndump            (nd_open)
 *   3. ioctl ATTACH { pid }       -> cookie, module pins the mm
 *   4. --vma-list ? dump map and exit
 *   5. ADD_REGION for each -r;  none given -> whole address space
 *   6. elf_begin (or raw file header)
 *   7. START, then loop read():
 *        - parse ndump_rec frames
 *        - DATA -> write payload, record PT_LOAD
 *        - ZERO -> write zeroes (or seek, in --raw sparse mode)
 *        - SKIP -> record hole only
 *   8. STAT, DETACH, elf_finish, close
 *
 * NO-LIBC DETAILS
 *   - entry symbol is `_start`, not `main`; argc/argv are taken off the
 *     stack in asm, since there is no crt0 to do it.
 *   - no printf: a minimal itoa/hex writer over nd_write(1, ...).
 *   - exit via nd_exit_group(); returning from _start is undefined without
 *     libc.
 *   - a static .bss + a fixed 1 MiB bounce buffer, no malloc.
 *
 * TODO: implement.
 */
