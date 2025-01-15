// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" "-cheri-compartment=example" -std=c2x | FileCheck %s
int foo(void);

[[cheriot::minimum_stack(256)]]
// CHECK: _Z8disabledv()
// CHECK-SAME: #0
__attribute__((cheri_compartment("example")))
int disabled(void)
{
	return foo();
}

[[cheriot::minimum_stack(256)]]
// CHECK-LABEL: @_Z9readback1v()
// CHECK: ret i32 256
__attribute__((cheri_compartment("example")))
int readback1(void)
{
	return __cheriot_minimum_stack__;
}

// CHECK-LABEL: attributes #0 =
// CHECK-SAME: "minimum-stack-size"="256"
