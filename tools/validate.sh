#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# tools/validate.sh — differential validation of nimbledump.
#
# Reference data comes from tools/selfmap (the target hashing its own known
#  regions), never from /proc/<pid>/mem or process_vm_readv. That keeps the
# oracle independent of the interface under test.
#
# Usage: ./validate.sh [--keep]
#
# Steps:
#   1. build selfmap, start it, capture its VMA ranges + content hashes
#   2. insmod nimbledump.ko
#   3. ndump -p <pid> --vma-list      -> compare geometry
#   4. ndump -p <pid> -o out.core     -> compare content hashes
#   5. ndump -p <pid> -r <range> -o r.bin -> single-region path
#   6. rmmod, report
#
# Exit codes: 0 pass, 1 mismatch, 2 environment problem (no module, no root).
#
# NOTE: step 4 must run while selfmap is still alive; the mm is pinned by the
# module during the run but page contents are not guaranteed after exit.

set -eu
echo "scaffold: implementation pending"
exit 2
