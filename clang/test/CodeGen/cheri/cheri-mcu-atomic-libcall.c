// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-cheriotrtos" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Wno-atomic-alignment" "-cheri-compartment=example" -S | FileCheck %s

_Atomic(int) x;

int callFromNotLibcall(void) {
  // Check that atomic libcalls get the right calling convention at the call site.
  // CHECK: auipcc  ct2, %cheriot_compartment_hi(__library_import_libcalls___atomic_fetch_add_4)
  // CHECK: clc     ct2, %cheriot_compartment_lo_i(.LBB0_2)(ct2)
  // CHECK: cjalr   ct2
  return __c11_atomic_fetch_add(&x, 1, 5);
}
