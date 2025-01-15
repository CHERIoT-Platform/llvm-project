// RUN: %clang -target riscv32cheriot -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,ALL
// RUN: %clang -target riscv32cheriot-unknown-unknown -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,ALL
// RUN: %clang -target riscv32cheriot-unknown-cheriotrtos -### -c %s 2>&1 | FileCheck %s -check-prefixes RTOS,ALL


// ALL: "-target-cpu" "cheriot"
// ALL: "-target-feature" "+e"
// ALL: "-target-feature" "+m"
// ALL: "-target-feature" "+c"
// ALL: "-target-feature" "+xcheri"

// BAREMETAL: "-target-abi" "cheriot-baremetal"
// RTOS: "-target-abi" "cheriot"
