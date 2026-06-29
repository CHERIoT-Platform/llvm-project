// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-target-feature" "+xcheriot" "-Oz" "-Werror" -std=c2x | FileCheck %s

struct Uart {};

// CHECK: @uart_WcmRg = external addrspace(200) global %struct.Uart, align 1 #0
__attribute__((cheriot_mmio("uart", "WcmRg"))) volatile struct Uart uart_WcmRg;

// CHECK: @uart_cmRWg = external addrspace(200) global %struct.Uart, align 1 #0
__attribute__((cheriot_mmio("uart", "cmRWg"))) volatile struct Uart uart_cmRWg;

// CHECK: @uart_RmcWg = external addrspace(200) global %struct.Uart, align 1 #0
__attribute__((cheriot_mmio("uart", "RmcWg"))) volatile struct Uart uart_RmcWg;

// CHECK: @uart_RmcWg2 = external addrspace(200) global %struct.Uart, align 1 #0
__attribute__((cheriot_mmio("uart", "RmcWg"))) volatile struct Uart uart_RmcWg2;

// CHECK: @SO_RWcmg = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "RWcmg"))) extern int SO_RWcmg;

// CHECK: @SO_RWmcg = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "RWmcg"))) extern int SO_RWmcg;

// CHECK: @SO_RmcWg = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "RmcWg"))) extern int SO_RmcWg;

// CHECK: @SO_mRcWg = external addrspace(200) global i32, align 4 #1
__attribute__((cheriot_shared_object("SO", "mRcWg"))) extern int SO_mRcWg;

// CHECK: @uart_R = external addrspace(200) global %struct.Uart, align 1 #2
__attribute__((cheriot_mmio("uart", "R"))) extern volatile struct Uart uart_R;

// CHECK: @uart_Rc = external addrspace(200) global %struct.Uart, align 1 #3
__attribute__((cheriot_mmio("uart", "Rc"))) extern volatile struct Uart uart_Rc;

// CHECK: @uart_cR = external addrspace(200) global %struct.Uart, align 1 #3
__attribute__((cheriot_mmio("uart", "cR"))) extern volatile struct Uart uart_cR;

// CHECK: @uart_Rcm = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "Rcm"))) extern volatile struct Uart uart_Rcm;

// CHECK: @uart_cRm = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "cRm"))) extern volatile struct Uart uart_cRm;

// CHECK: @uart_cmR = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "cmR"))) extern volatile struct Uart uart_cmR;

// CHECK: @uart_mRc = external addrspace(200) global %struct.Uart, align 1 #4
__attribute__((cheriot_mmio("uart", "mRc"))) extern volatile struct Uart uart_mRc;

// CHECK: @uart_W = external addrspace(200) global %struct.Uart, align 1 #5
__attribute__((cheriot_mmio("uart", "W"))) extern volatile struct Uart uart_W;

// CHECK: @uart_Wc = external addrspace(200) global %struct.Uart, align 1 #6
__attribute__((cheriot_mmio("uart", "Wc"))) extern volatile struct Uart uart_Wc;

// CHECK: @uart_cW = external addrspace(200) global %struct.Uart, align 1 #6
__attribute__((cheriot_mmio("uart", "cW"))) extern volatile struct Uart uart_cW;

// CHECK: @uart_RWc = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "RWc"))) extern volatile struct Uart uart_RWc;

// CHECK: @uart_cRW = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "cRW"))) extern volatile struct Uart uart_cRW;

// CHECK: @uart_RcW = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "RcW"))) extern volatile struct Uart uart_RcW;

// CHECK: @uart_WRc = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "WRc"))) extern volatile struct Uart uart_WRc;

// CHECK: @uart_WcR = external addrspace(200) global %struct.Uart, align 1 #7
__attribute__((cheriot_mmio("uart", "WcR"))) extern volatile struct Uart uart_WcR;

// CHECK: @uart_Rcg = external addrspace(200) global %struct.Uart, align 1 #8
__attribute__((cheriot_mmio("uart", "Rcg"))) extern volatile struct Uart uart_Rcg;

// CHECK: @uart_cRg = external addrspace(200) global %struct.Uart, align 1 #8
__attribute__((cheriot_mmio("uart", "cRg"))) extern volatile struct Uart uart_cRg;

// CHECK: @uart_cgR = external addrspace(200) global %struct.Uart, align 1 #8
__attribute__((cheriot_mmio("uart", "cgR"))) extern volatile struct Uart uart_cgR;

// CHECK: @uart_gcR = external addrspace(200) global %struct.Uart, align 1 #8
__attribute__((cheriot_mmio("uart", "gcR"))) extern volatile struct Uart uart_gcR;

// CHECK: @uart_gRc = external addrspace(200) global %struct.Uart, align 1 #8
__attribute__((cheriot_mmio("uart", "gcR"))) extern volatile struct Uart uart_gRc;

// CHECK: @SO_R = external addrspace(200) global i32, align 4 #9
__attribute__((cheriot_shared_object("SO", "R"))) extern int SO_R;

// CHECK: @SO_Rc = external addrspace(200) global i32, align 4 #10
__attribute__((cheriot_shared_object("SO", "Rc"))) extern int SO_Rc;

// CHECK: @SO_cR = external addrspace(200) global i32, align 4 #10
__attribute__((cheriot_shared_object("SO", "cR"))) extern int SO_cR;

// CHECK: @SO_Rcm = external addrspace(200) global i32, align 4 #11
__attribute__((cheriot_shared_object("SO", "Rcm"))) extern int SO_Rcm;

// CHECK: @SO_cRm = external addrspace(200) global i32, align 4 #11
__attribute__((cheriot_shared_object("SO", "cRm"))) extern int SO_cRm;

// CHECK: @SO_cmR = external addrspace(200) global i32, align 4 #11
__attribute__((cheriot_shared_object("SO", "cmR"))) extern int SO_cmR;

// CHECK: @SO_mRc = external addrspace(200) global i32, align 4 #11
__attribute__((cheriot_shared_object("SO", "mRc"))) extern int SO_mRc;

// CHECK: @SO_W = external addrspace(200) global i32, align 4 #12
__attribute__((cheriot_shared_object("SO", "W"))) extern int SO_W;

// CHECK: @SO_Wc = external addrspace(200) global i32, align 4 #13
__attribute__((cheriot_shared_object("SO", "Wc"))) extern int SO_Wc;

// CHECK: @SO_cW = external addrspace(200) global i32, align 4 #13
__attribute__((cheriot_shared_object("SO", "cW"))) extern int SO_cW;

// CHECK: @SO_RWc = external addrspace(200) global i32, align 4 #14
__attribute__((cheriot_shared_object("SO", "RWc"))) extern int SO_RWc;

// CHECK: @SO_cRW = external addrspace(200) global i32, align 4 #14
__attribute__((cheriot_shared_object("SO", "cRW"))) extern int SO_cRW;

// CHECK: @SO_RcW = external addrspace(200) global i32, align 4 #14
__attribute__((cheriot_shared_object("SO", "RcW"))) extern int SO_RcW;

// CHECK: @SO_WRc = external addrspace(200) global i32, align 4 #14
__attribute__((cheriot_shared_object("SO", "WRc"))) extern int SO_WRc;

// CHECK: @SO_WcR = external addrspace(200) global i32, align 4 #14
__attribute__((cheriot_shared_object("SO", "WcR"))) extern int SO_WcR;

// CHECK: @SO_Rcg = external addrspace(200) global i32, align 4 #15
__attribute__((cheriot_shared_object("SO", "Rcg"))) extern int SO_Rcg;

// CHECK: @SO_cRg = external addrspace(200) global i32, align 4 #15
__attribute__((cheriot_shared_object("SO", "cRg"))) extern int SO_cRg;

// CHECK: @SO_cgR = external addrspace(200) global i32, align 4 #15
__attribute__((cheriot_shared_object("SO", "cgR"))) extern int SO_cgR;

// CHECK: @SO_gcR = external addrspace(200) global i32, align 4 #15
__attribute__((cheriot_shared_object("SO", "gcR"))) extern int SO_gcR;

// CHECK: @SO_gRc = external addrspace(200) global i32, align 4 #15
__attribute__((cheriot_shared_object("SO", "gRc"))) extern int SO_gRc;


struct MMIOWithField { int field; };

// CHECK: @mmio = external addrspace(200) global %struct.MMIOWithField, align 4 #16
__attribute__((cheriot_mmio("mmio", "R"))) extern volatile struct MMIOWithField mmio;

void doSomethingWithUart(volatile struct Uart *uart);
void doSomethingWithSO(int *SO);
void doSomethingWithField(int);

void func() {
  doSomethingWithUart(&uart_WcmRg);
  doSomethingWithUart(&uart_cmRWg);
  doSomethingWithUart(&uart_RmcWg);
  doSomethingWithUart(&uart_RmcWg2);

  doSomethingWithSO(&SO_RWcmg);
  doSomethingWithSO(&SO_RWmcg);
  doSomethingWithSO(&SO_RmcWg);
  doSomethingWithSO(&SO_mRcWg);

  doSomethingWithUart(&uart_R);
  doSomethingWithUart(&uart_Rc);
  doSomethingWithUart(&uart_cR);
  doSomethingWithUart(&uart_Rcm);
  doSomethingWithUart(&uart_cRm);
  doSomethingWithUart(&uart_cmR);
  doSomethingWithUart(&uart_mRc);
  doSomethingWithUart(&uart_W);
  doSomethingWithUart(&uart_Wc);
  doSomethingWithUart(&uart_cW);
  doSomethingWithUart(&uart_RWc);
  doSomethingWithUart(&uart_cRW);
  doSomethingWithUart(&uart_RcW);
  doSomethingWithUart(&uart_WRc);
  doSomethingWithUart(&uart_WcR);
  doSomethingWithUart(&uart_Rcg);
  doSomethingWithUart(&uart_cRg);
  doSomethingWithUart(&uart_cgR);
  doSomethingWithUart(&uart_gcR);
  doSomethingWithUart(&uart_gRc);

  doSomethingWithSO(&SO_R);
  doSomethingWithSO(&SO_Rc);
  doSomethingWithSO(&SO_cR);
  doSomethingWithSO(&SO_Rcm);
  doSomethingWithSO(&SO_cRm);
  doSomethingWithSO(&SO_cmR);
  doSomethingWithSO(&SO_mRc);
  doSomethingWithSO(&SO_W);
  doSomethingWithSO(&SO_Wc);
  doSomethingWithSO(&SO_cW);
  doSomethingWithSO(&SO_RWc);
  doSomethingWithSO(&SO_cRW);
  doSomethingWithSO(&SO_RcW);
  doSomethingWithSO(&SO_cW);
  doSomethingWithSO(&SO_WRc);
  doSomethingWithSO(&SO_WcR);
  doSomethingWithSO(&SO_WcR);
  doSomethingWithSO(&SO_Rcg);
  doSomethingWithSO(&SO_cRg);
  doSomethingWithSO(&SO_cgR);
  doSomethingWithSO(&SO_gcR);
  doSomethingWithSO(&SO_gRc);

// CHECK:  %0 = load volatile i32, ptr addrspace(200) @mmio, align 4
// CHECK-NEXT:  tail call addrspace(200) void @doSomethingWithField(i32 noundef %0) #19
  doSomethingWithField(mmio.field);
// CHECK-NEXT:  store volatile i32 10, ptr addrspace(200) @mmio, align 4
  mmio.field = 10;

// CHECK:  %1 = load volatile i32, ptr addrspace(200) @mmio, align 4
// CHECK-NEXT:  tail call addrspace(200) void @doSomethingWithField(i32 noundef %1) #19
  doSomethingWithField((&mmio)->field);
// CHECK-NEXT:  store volatile i32 10, ptr addrspace(200) @mmio, align 4
  (&mmio)->field = 10;

}


// CHECK: attributes #0 = { "cheriot_global_cap_import"="mem,uart,RWcmg" }
// CHECK: attributes #1 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,RWcmg" }
// CHECK: attributes #2 = { "cheriot_global_cap_import"="mem,uart,R----" }
// CHECK: attributes #3 = { "cheriot_global_cap_import"="mem,uart,R-c--" }
// CHECK: attributes #4 = { "cheriot_global_cap_import"="mem,uart,R-cm-" }
// CHECK: attributes #5 = { "cheriot_global_cap_import"="mem,uart,-W---" }
// CHECK: attributes #6 = { "cheriot_global_cap_import"="mem,uart,-Wc--" }
// CHECK: attributes #7 = { "cheriot_global_cap_import"="mem,uart,RWc--" }
// CHECK: attributes #8 = { "cheriot_global_cap_import"="mem,uart,R-c-g" }
// CHECK: attributes #9 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,R----" }
// CHECK: attributes #10 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,R-c--" }
// CHECK: attributes #11 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,R-cm-" }
// CHECK: attributes #12 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,-W---" }
// CHECK: attributes #13 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,-Wc--" }
// CHECK: attributes #14 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,RWc--" }
// CHECK: attributes #15 = { "cheriot_global_cap_import"="cheriot_shared_object,SO,R-c-g" }
// CHECK: attributes #16 = { "cheriot_global_cap_import"="mem,mmio,R----" }
// CHECK: attributes #17 = { minsize nounwind optsize "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+e,+m,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul" }
// CHECK: attributes #18 = { minsize optsize "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+e,+m,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul" }
// CHECK: attributes #19 = { minsize nounwind optsize }
