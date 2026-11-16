// RUN: %clang_cc1 -triple riscv32-unknown-cheriotrtos -target-abi cheriot \
// RUN:   -target-feature +xcheriot -emit-llvm -o - %s | FileCheck %s --check-prefix=INBOUNDS
// RUN: %clang_cc1 -triple riscv32-unknown-cheriotrtos -target-abi cheriot \
// RUN:   -target-feature +xcheriot -fwrapv-pointer -emit-llvm -o - %s | FileCheck %s --check-prefix=WRAP
// RUN: %clang_cc1 -triple riscv32-unknown-cheriotrtos -target-abi cheriot \
// RUN:   -target-feature +xcheriot -fwrapv -emit-llvm -o - %s | FileCheck %s --check-prefix=INBOUNDS

// Only -fwrapv-pointer should drop inbounds; -fwrapv alone shouldn't.

// INBOUNDS-LABEL: define {{.*}} @up(
// INBOUNDS: getelementptr inbounds i8, ptr addrspace(200)
// WRAP-LABEL: define {{.*}} @up(
// WRAP-NOT: getelementptr inbounds
// WRAP: getelementptr i8, ptr addrspace(200)
char *up(char *p) { return __builtin_align_up(p, 16); }
