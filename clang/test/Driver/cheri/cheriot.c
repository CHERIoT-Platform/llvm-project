// RUN: %clang -target riscv32cheriot-unknown-unknown -mcpu=cheriot -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,ALL
// RUN: %clang -target riscv32cheriot-unknown-unknown -mabi=cheriot-baremetal -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,ALL
// RUN: %clang -target riscv32cheriot-unknown-cheriotrtos -### -c %s 2>&1 | FileCheck %s -check-prefixes RTOS,ALL


// ALL: "-target-cpu" "cheriot"
// ALL: "-target-feature" "+xcheriot"

// BAREMETAL: "-target-abi" "cheriot-baremetal"
// RTOS: "-target-abi" "cheriot"
