// REQUIRES: riscv
// RUN: llvm-mc -triple riscv32-unknown-cheriotrtos -filetype=obj -o %t %s
// RUN: ld.lld %t %S/Inputs/cheriot-capalign.lds -o %t2
// RUN: llvm-readobj --sections %t2 | FileCheck %s

// Verify that CAPALIGN directives in the linker script actually
// force higher alignment on an over-sized section.

// CHECK:          Index: 2
// CHECK-NEXT:     Name: .bar (7)
// CHECK-NEXT:     Type: SHT_PROGBITS (0x1)
// CHECK-NEXT:     Flags [ (0x0)
// CHECK-NEXT:     ]
// CHECK-NEXT:     Address: 0x0
// CHECK-NEXT:     Offset: 0x20000
// CHECK-NEXT:     Size: 1179648
// CHECK-NEXT:     Link: 0
// CHECK-NEXT:     Info: 0
// CHECK-NEXT:     AddressAlignment: 131072
// CHECK-NEXT:     EntrySize: 0

.section .bar
.byte 0