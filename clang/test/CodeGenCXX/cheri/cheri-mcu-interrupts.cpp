// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" "-cheri-compartment=example" | FileCheck %s
int foo(void);

// CHECK: define dso_local noundef i32 @_Z8disabledv() local_unnamed_addr addrspace(200) #[[DIS:[0-9]]]
[[cheriot::interrupt_state(disabled)]]
int disabled(void)
{
	return foo();
}

// CHECK: define dso_local noundef i32 @_Z7enabledv() local_unnamed_addr addrspace(200) #[[EN:[0-9]]]
[[cheriot::interrupt_state(enabled)]]
int enabled(void)
{
	return foo();
}

// CHECK: define dso_local noundef i32 @_Z7inheritv() local_unnamed_addr addrspace(200) #[[INH:[0-9]]]
[[cheriot::interrupt_state(inherit)]]
int inherit(void)
{
	return foo();
}

// CHECK: define dso_local noundef i32 @_Z9disabled1v() local_unnamed_addr addrspace(200) #[[DIS]]
[[cheri::interrupt_state(disabled)]]
int disabled1(void)
{
	return foo();
}

// CHECK: define dso_local noundef i32 @_Z8enabled1v() local_unnamed_addr addrspace(200) #[[EN]]
[[cheri::interrupt_state(enabled)]]
int enabled1(void)
{
	return foo();
}

// CHECK: define dso_local noundef i32 @_Z8inherit1v() local_unnamed_addr addrspace(200) #[[INH]]
[[cheri::interrupt_state(inherit)]]
int inherit1(void)
{
	return foo();
}

// CHECK: attributes #[[DIS]]
// CHECK-SAME: "interrupt-state"="disabled"
// CHECK: attributes #[[EN]]
// CHECK-SAME: "interrupt-state"="enabled"
// CHECK: attributes #[[INH]]
// CHECK-SAME: "interrupt-state"="inherit"
