// SPDX-License-Identifier: GPL-2.0
//
// src/scan.rs — cheap post-processing over the drained stream.
//
//   scan_pattern(buf, needle)
//       Boyer-Moore-Horspool over the bounce buffer. Reports offsets;
//       used by --scan to locate keys/strings without a full dump-to-disk
//       pass. Deliberately dumb: a real matcher would need the whole address
//       space resident, which defeats the streaming design.
//
//   scan_entropy(buf)
//       Shannon entropy per 1 KiB block; flags packed/encrypted regions.
//       Useful for triaging which VMA is worth a targeted second pass.
//
// Both functions operate on the caller's bounce buffer only and never
// retain state between records, so a 40 GiB target is scannable in constant
// memory.
#![allow(dead_code)]

/// Returns the offset of the first match in `buf`, or None.
pub fn pattern(buf: &[u8], needle: &[u8]) -> Option<usize> {
    // TODO: implement — BMH: build bad-char shift table over needle
    //   (needs a fixed 256-entry stack array, no heap), then walk
    //   buf from the end of the window backwards.
    let _ = (buf, needle);
    unimplemented!()
}

/// Shannon entropy in bits/byte for `buf` (0.0 = uniform, 8.0 = packed).
pub fn entropy(buf: &[u8]) -> f64 {
    // TODO: implement — 256-entry u64 histogram on the stack,
    //   then -sum(p_i * log2(p_i)) over non-zero bins.
    //   f64 ops are soft-float-free on x86_64 (SSE2), no libm needed
    //   if log2 is computed via a simple frexp-style exponent trick.
    let _ = buf;
    unimplemented!()
}
