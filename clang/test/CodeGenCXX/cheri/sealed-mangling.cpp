// RUN: %riscv64_cheri_purecap_cc1 -emit-llvm -S -o - %s | FileCheck %s

// CHECK: @_Z5test1U19__sealed_capabilityPi
int * __sealed_capability test1(int * __sealed_capability a) {
    return a;
}
