// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-cheriotrtos" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-target-feature" "+xcheriot" "-Oz" "-Wno-atomic-alignment" "-cheri-compartment=example" -S | FileCheck %s

_Atomic(int) x;

int callFromNotLibcall(void) {
  // Check that atomic libcalls get the right calling convention at the call site.
  // CHECK: auipcc  t2, %cheriot_compartment_code_hi(__library_import_libcalls___atomic_fetch_add_4)
  // CHECK: clc     t2, %cheriot_compartment_lo_i(.LBB0_2)(t2)
  // CHECK: cjalr   t2
  return __c11_atomic_fetch_add(&x, 1, 5);
}
