// RUN: %clang -target riscv32-unknown-unknown -mcpu=cheriot -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,CHERIOT,ALL
// RUN: %clang -target riscv32-unknown-unknown -mabi=cheriot-baremetal -### -c %s 2>&1 | FileCheck %s -check-prefixes BAREMETAL,CHERIOT,ALL
// RUN: %clang -target riscv32-unknown-cheriotrtos -### -c %s 2>&1 | FileCheck %s -check-prefixes RTOS,CHERIOT,ALL
// RUN: %clang -target riscv32-unknown-unknown -mcpu=cheriot-ibex -### -c %s 2>&1 | FileCheck %s -check-prefixes IBEX,ALL
// RUN: %clang -target riscv32-unknown-unknown -mcpu=cheriot-kudu -### -c %s 2>&1 | FileCheck %s -check-prefixes KUDU,ALL

// CHERIOT: "-target-cpu" "cheriot"
// IBEX: "-target-cpu" "cheriot-ibex"
// KUDU: "-target-cpu" "cheriot-kudu"

// CHERIOT-NOT: "-target-feature" "+b"
// IBEX: "-target-feature" "+b"
// KUDU: "-target-feature" "+b"

// CHERIOT-NOT: "-target-feature" "+zbkb"
// IBEX: "-target-feature" "+zbkb"
// KUDU: "-target-feature" "+zbkb"

// CHERIOT-NOT: "-target-feature" "+zbkc"
// IBEX: "-target-feature" "+zbkc"
// KUDU: "-target-feature" "+zbkc"

// ALL: "-target-feature" "+xcheriot"

// BAREMETAL: "-target-abi" "cheriot-baremetal"
// RTOS: "-target-abi" "cheriot"
