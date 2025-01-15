// RUN: %clang_cc1 %s -o - -triple riscv32-unknown-unknown -emit-llvm -mframe-pointer=none -mcmodel=small -target-cpu cheriot -target-feature +xcheri -target-feature -64bit -target-feature -relax -target-feature -xcheri-rvc -target-feature -save-restore -target-abi cheriot -Oz -cheri-compartment=example -Wcheri-compartment-return-void -verify -fsyntax-only
// RUN: %clang_cc1 %s -o - -triple riscv32-unknown-unknown -emit-llvm -mframe-pointer=none -mcmodel=small -target-cpu cheriot -target-feature +xcheri -target-feature -64bit -target-feature -relax -target-feature -xcheri-rvc -target-feature -save-restore -target-abi cheriot -Oz -cheri-compartment=example -Wcheri-compartment-return-void -fdiagnostics-parseable-fixits -fsyntax-only 2>&1 | FileCheck %s

__attribute__((cheri_compartment("example"))) void void_return_type_f(int a) // expected-warning{{void return on a cross-compartment call makes it impossible for callers to detect failure}} expected-note{{replace void return type with int}}
{
  if (a) {
    /// CHECK: fix-it:"{{.*}}":{[[@LINE+1]]:[[COL:[0-9]+]]-[[@LINE+1]]:[[COL]]}:" 0"
    return; // expected-warning{{cross-compartement calls that always succeed should return 0 instead}}
  }
} // expected-warning{{cross-compartement calls that always succeed should return 0 instead}}

__attribute__((cheri_compartment("example"))) int int_return_type_f() {
  return 0;
}

void unused_int_return_type_f() {
  int_return_type_f(); // expected-warning{{ignoring return value of function declared with 'nodiscard' attribute: CHERI compartment call}}
}
