# RUN: not llvm-mc %s -triple=riscv64 -mattr=+xcheri -show-encoding 2>%t.err | FileCheck %s
# RUN: FileCheck %s -check-prefix=ERRORS < %t.err

## The null capability register is accepted as a general-purpose capability
## operand, but DDC is not -- DDC is only valid where the operand class
## explicitly allows it (e.g. cbuildcap).

cincoffset c1, cnull, 12
# CHECK: cincoffset	ra, zero, 12            # encoding: [0xdb,0x10,0xc0,0x00]

cincoffset c1, ddc, 13
# ERRORS: [[@LINE-1]]:16: error: invalid operand for instruction

csetaddr c1, ddc, x2
# ERRORS: [[@LINE-1]]:14: error: invalid operand for instruction

## DDC is valid where the operand class allows it:
cbuildcap c1, ddc, c2
# CHECK: cbuildcap	ra, ddc, sp             # encoding: [0xdb,0x00,0x20,0x3a]
