// RUN: %clang -target riscv32cheriot-unknown-unknown -mcpu=cheriot -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,CHERIOT,ALL
// RUN: %clang -target riscv32cheriot-unknown-unknown -mabi=cheriot-baremetal -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,CHERIOT,ALL
// RUN: %clang -target riscv32cheriot-unknown-cheriotrtos -### -c %s 2>&1 | FileCheck %s -check-prefixes RTOS,CHERIOT,ALL
// RUN: %clang -target riscv32cheriot-unknown-unknown -mcpu=cheriot-ibex -### -c %s 2>&1 | FileCheck %s -check-prefixes IBEX,ALL
// RUN: %clang -target riscv32cheriot-unknown-unknown -mcpu=cheriot-kudu -### -c %s 2>&1 | FileCheck %s -check-prefixes KUDU,ALL

// CHERIOT: "-target-cpu" "cheriot"
// IBEX: "-target-cpu" "cheriot-ibex"
// KUDU: "-target-cpu" "cheriot-kudu"

// CHERIOT-NOT: "-target-feature" "+b"
// IBEX: "-target-feature" "+b"
// KUDU: "-target-feature" "+b"

// ALL: "-target-feature" "+xcheriot"

// BAREMETAL: "-target-abi" "cheriot-baremetal"
// RTOS: "-target-abi" "cheriot"
