// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-unknown" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" -std=c2x | FileCheck %s

struct Uart {};

// No specific perm encoding means "RWcm".
// CHECK: @uart = external addrspace(200) global %struct.Uart, align 1 #0 
__attribute__((cheriot_mmio("uart"))) extern volatile struct Uart uart;

// CHECK: @uart1 = external addrspace(200) global %struct.Uart, align 1 #0 
__attribute__((cheriot_mmio("uart", "WcmR"))) volatile struct Uart uart1;

// CHECK: @uart2 = external addrspace(200) global %struct.Uart, align 1 #0 
__attribute__((cheriot_mmio("uart", "cmRW"))) volatile struct Uart uart2;

// CHECK: @uart3 = external addrspace(200) global %struct.Uart, align 1 #0 
__attribute__((cheriot_mmio("uart", "RmcW"))) volatile struct Uart uart3;

// CHECK: @uart4 = external addrspace(200) global %struct.Uart, align 1 #0 
__attribute__((cheriot_mmio("uart", "RmcW"))) volatile struct Uart uart4;

// CHECK: @SO = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO"))) int SO;

// CHECK: @SO1 = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "RWcm"))) extern int SO1;

// CHECK: @SO2 = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "RWmc"))) extern int SO2;

// CHECK: @SO3 = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "RmcW"))) extern int SO3;

// CHECK: @SO4 = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "mRcW"))) extern int SO4;

// CHECK: @uart5 = external addrspace(200) global %struct.Uart, align 1 #2
__attribute__((cheriot_mmio("uart", "R"))) extern volatile struct Uart uart5;

// CHECK: @uart6 = external addrspace(200) global %struct.Uart, align 1 #3
__attribute__((cheriot_mmio("uart", "Rc"))) extern volatile struct Uart uart6;

// CHECK: @uart61 = external addrspace(200) global %struct.Uart, align 1 #3
__attribute__((cheriot_mmio("uart", "cR"))) extern volatile struct Uart uart61;

// CHECK: @uart7 = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "Rcm"))) extern volatile struct Uart uart7;

// CHECK: @uart71 = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "cRm"))) extern volatile struct Uart uart71;

// CHECK: @uart72 = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "cmR"))) extern volatile struct Uart uart72;

// CHECK: @uart73 = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "mRc"))) extern volatile struct Uart uart73;

// CHECK: @uart8 = external addrspace(200) global %struct.Uart, align 1 #5
__attribute__((cheriot_mmio("uart", "W"))) extern volatile struct Uart uart8;

// CHECK: @uart9 = external addrspace(200) global %struct.Uart, align 1 #6
__attribute__((cheriot_mmio("uart", "Wc"))) extern volatile struct Uart uart9;

// CHECK: @uart91 = external addrspace(200) global %struct.Uart, align 1 #6
__attribute__((cheriot_mmio("uart", "cW"))) extern volatile struct Uart uart91;

// CHECK: @uart10 = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "RWc"))) extern volatile struct Uart uart10;

// CHECK: @uart101 = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "cRW"))) extern volatile struct Uart uart101;

// CHECK: @uart102 = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "RcW"))) extern volatile struct Uart uart102;

// CHECK: @uart103 = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "WRc"))) extern volatile struct Uart uart103;

// CHECK: @uart104 = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "WcR"))) extern volatile struct Uart uart104;

// CHECK: @SO5 = external addrspace(200) global i32, align 4 #8
__attribute__((cheriot_shared_object("SO", "R"))) extern int SO5;

// CHECK: @SO6 = external addrspace(200) global i32, align 4 #9
__attribute__((cheriot_shared_object("SO", "Rc"))) extern int SO6;

// CHECK: @SO61 = external addrspace(200) global i32, align 4 #9
__attribute__((cheriot_shared_object("SO", "cR"))) extern int SO61;

// CHECK: @SO7 = external addrspace(200) global i32, align 4 #10
__attribute__((cheriot_shared_object("SO", "Rcm"))) extern int SO7;

// CHECK: @SO71 = external addrspace(200) global i32, align 4 #10
__attribute__((cheriot_shared_object("SO", "cRm"))) extern int SO71;

// CHECK: @SO72 = external addrspace(200) global i32, align 4 #10
__attribute__((cheriot_shared_object("SO", "cmR"))) extern int SO72;

// CHECK: @SO73 = external addrspace(200) global i32, align 4 #10
__attribute__((cheriot_shared_object("SO", "mRc"))) extern int SO73;

// CHECK: @SO8 = external addrspace(200) global i32, align 4 #11
__attribute__((cheriot_shared_object("SO", "W"))) extern int SO8;

// CHECK: @SO9 = external addrspace(200) global i32, align 4 #12
__attribute__((cheriot_shared_object("SO", "Wc"))) extern int SO9;

// CHECK: @SO91 = external addrspace(200) global i32, align 4 #12
__attribute__((cheriot_shared_object("SO", "cW"))) extern int SO91;

// CHECK: @SO10 = external addrspace(200) global i32, align 4 #13
__attribute__((cheriot_shared_object("SO", "RWc"))) extern int SO10;

// CHECK: @SO101 = external addrspace(200) global i32, align 4 #13
__attribute__((cheriot_shared_object("SO", "cRW"))) extern int SO101;

// CHECK: @SO102 = external addrspace(200) global i32, align 4 #13
__attribute__((cheriot_shared_object("SO", "RcW"))) extern int SO102;

// CHECK: @SO103 = external addrspace(200) global i32, align 4 #13
__attribute__((cheriot_shared_object("SO", "WRc"))) extern int SO103;

// CHECK: @SO104 = external addrspace(200) global i32, align 4 #13
__attribute__((cheriot_shared_object("SO", "WcR"))) extern int SO104;

void doSomethingWithUart(volatile struct Uart *uart);
void doSomethingWithSO(int *SO);

void func() {
  doSomethingWithUart(&uart);
  doSomethingWithUart(&uart1);
  doSomethingWithUart(&uart2);
  doSomethingWithUart(&uart3);
  doSomethingWithUart(&uart4);

  doSomethingWithSO(&SO);
  doSomethingWithSO(&SO1);
  doSomethingWithSO(&SO2);
  doSomethingWithSO(&SO3);
  doSomethingWithSO(&SO4);

  doSomethingWithUart(&uart5);
  doSomethingWithUart(&uart6);
  doSomethingWithUart(&uart61);
  doSomethingWithUart(&uart7);
  doSomethingWithUart(&uart71);
  doSomethingWithUart(&uart72);
  doSomethingWithUart(&uart73);
  doSomethingWithUart(&uart8);
  doSomethingWithUart(&uart9);
  doSomethingWithUart(&uart91);
  doSomethingWithUart(&uart10);
  doSomethingWithUart(&uart101);
  doSomethingWithUart(&uart102);
  doSomethingWithUart(&uart103);
  doSomethingWithUart(&uart104);

  doSomethingWithSO(&SO5);
  doSomethingWithSO(&SO6);
  doSomethingWithSO(&SO61);
  doSomethingWithSO(&SO7);
  doSomethingWithSO(&SO71);
  doSomethingWithSO(&SO72);
  doSomethingWithSO(&SO73);
  doSomethingWithSO(&SO8);
  doSomethingWithSO(&SO9);
  doSomethingWithSO(&SO91);
  doSomethingWithSO(&SO10);
  doSomethingWithSO(&SO101);
  doSomethingWithSO(&SO102);
  doSomethingWithSO(&SO103);
  doSomethingWithSO(&SO104);

}


// CHECK: attributes #0 = { "cheriot_global_cap_import"="mem,uart,RWcm" }
// CHECK: attributes #1 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,RWcm" }
// CHECK: attributes #2 = { "cheriot_global_cap_import"="mem,uart,R---" }
// CHECK: attributes #3 = { "cheriot_global_cap_import"="mem,uart,R-c-" }
// CHECK: attributes #4 = { "cheriot_global_cap_import"="mem,uart,R-cm" }
// CHECK: attributes #5 = { "cheriot_global_cap_import"="mem,uart,-W--" }
// CHECK: attributes #6 = { "cheriot_global_cap_import"="mem,uart,-Wc-" }
// CHECK: attributes #7 = { "cheriot_global_cap_import"="mem,uart,RWc-" }
// CHECK: attributes #8 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,R---" }
// CHECK: attributes #9 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,R-c-" }
// CHECK: attributes #10 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,R-cm" }
// CHECK: attributes #11 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,-W--" }
// CHECK: attributes #12 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,-Wc-" }
// CHECK: attributes #13 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,RWc-" }
