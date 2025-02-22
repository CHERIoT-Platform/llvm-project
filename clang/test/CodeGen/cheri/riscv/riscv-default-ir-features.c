// RUN: %clang --target=riscv32 -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,NOXCHERI,RV32-NOXCHERI
// RUN: %clang --target=riscv64 -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,NOXCHERI,RV64-NOXCHERI

// RUN: %clang --target=riscv32 -march=rv32ixcheri -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,XCHERI,RV32-XCHERI
// RUN: %clang --target=riscv64 -march=rv64ixcheri -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,XCHERI,RV64-XCHERI
// RUN: %clang --target=riscv32 -march=rv32ixcheri -S -mxcheri-rvc -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,XCHERI,RVC,RV32-XCHERI
// RUN: %clang --target=riscv64 -march=rv64ixcheri -S -mxcheri-rvc -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,XCHERI,RVC,RV64-XCHERI
// RUN: %clang --target=riscv32 -march=rv32ixcheri -S -mno-xcheri-rvc -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,XCHERI,NORVC,RV32-XCHERI
// RUN: %clang --target=riscv64 -march=rv64ixcheri -S -mno-xcheri-rvc -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,XCHERI,NORVC,RV64-XCHERI

/// Same checks for purecap
// RUN: %clang --target=riscv32 -march=rv32ixcheri -mabi=il32pc64 -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,XCHERI,RV32-XCHERI-PURECAP
// RUN: %clang --target=riscv64 -march=rv64ixcheri -mabi=l64pc128 -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,XCHERI,RV64-XCHERI-PURECAP
// RUN: %clang --target=riscv32 -march=rv32ixcheri -mabi=il32pc64 -S -mxcheri-rvc -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,XCHERI,RVC,RV32-XCHERI-PURECAP
// RUN: %clang --target=riscv64 -march=rv64ixcheri -mabi=l64pc128 -S -mxcheri-rvc -emit-llvm %s -o - \
// RUN:    | FileCheck %s --check-prefixes=CHECK,RV64,XCHERI,RVC,RV64-XCHERI-PURECAP
// RUN: %clang --target=riscv32 -march=rv32ixcheri -mabi=il32pc64 -S -mno-xcheri-rvc -emit-llvm %s -o - \
// RUN:    | FileCheck %s --check-prefixes=CHECK,RV32,XCHERI,NORVC,RV32-XCHERI-PURECAP
// RUN: %clang --target=riscv64 -march=rv64ixcheri -mabi=l64pc128 -S -mno-xcheri-rvc -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,XCHERI,NORVC,RV64-XCHERI-PURECAP

/// Enabling -mxcheri-norvc with a non-CHERI triple shouldn't enable the CHERI datalayout
// RUN: %clang --target=riscv32 -mxcheri-norvc -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,NOXCHERI,RV32-NOCHERI
// RUN: %clang --target=riscv64 -mxcheri-norvc -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,NOXCHERI,RV64-NOCHERI
// RUN: %clang --target=riscv32 -mno-xcheri-norvc -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV32,NOXCHERI,RV32-NOCHERI
// RUN: %clang --target=riscv64 -mno-xcheri-norvc -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefixes=CHECK,RV64,NOXCHERI,RV64-NOCHERI

// RV32-NOCHERI: target datalayout = "e-m:e-p:32:32-i64:64-n32-S128"
// RV64-NOCHERI: target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
// RV32-XCHERI: target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32"
// RV64-XCHERI: target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128-pf200:128:128:128:64"
// RV32-XCHERI-PURECAP: target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
// RV64-XCHERI-PURECAP: target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128-pf200:128:128:128:64-A200-P200-G200"

// LLVM lists all negative features, so we only check the positive ones and the negative ones we care about here.
// CHECK: "target-features"=

// RV32-SAME: +32bit,
// RV64-SAME: +64bit,
// XCHERI-SAME: +xcheri,
// NOXCHERI-SAME: -xcheri,
// RVC-SAME: -xcheri-norvc,
// NORVC-SAME: +xcheri-norvc,

// RV32-NOXCHERI: !{i32 1, !"target-abi", !"ilp32"}
// RV64-NOXCHERI: !{i32 1, !"target-abi", !"lp64"}
// RV32-XCHERI: !{i32 1, !"target-abi", !"ilp32"}
// RV64-XCHERI: !{i32 1, !"target-abi", !"lp64"}
// RV32-XCHERI-PURECAP: !{i32 1, !"target-abi", !"il32pc64"}
// RV64-XCHERI-PURECAP: !{i32 1, !"target-abi", !"l64pc128"}

int test(void) {
  return 1;
}
