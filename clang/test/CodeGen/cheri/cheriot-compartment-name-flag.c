// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-target-feature" "+xcheriot" "-Oz" "-Werror" "-cheri-compartment=example" | FileCheck %s

int test() { return 0; }

// Check that the compartment name was correct embedded in a module flag
// CHECK: !{i32 1, !"cheriot-compartment", !"example"}
