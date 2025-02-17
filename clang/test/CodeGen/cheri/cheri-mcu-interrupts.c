// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" "-cheri-compartment=example" | FileCheck %s
int foo(void);

// CHECK: define dso_local i32 @disabled() local_unnamed_addr addrspace(200) #[[DIS:[0-9]]]
__attribute__((cheriot_interrupt_state(disabled)))
int disabled(void)
{
	return foo();
}

// CHECK: define dso_local i32 @enabled() local_unnamed_addr addrspace(200) #[[EN:[0-9]]]
__attribute__((cheriot_interrupt_state(enabled)))
int enabled(void)
{
	return foo();
}

// CHECK: define dso_local i32 @inherit() local_unnamed_addr addrspace(200) #[[INH:[0-9]]]
__attribute__((cheriot_interrupt_state(inherit)))
int inherit(void)
{
	return foo();
}

// The default for exported functions should be interrupts enabled
//
// CHECK: define dso_local chericcallcce i32 @_Z21default_enable_calleev() local_unnamed_addr addrspace(200) #[[DEFEN:[0-9]]]
__attribute__((cheri_compartment("example")))
int default_enable_callee(void)
{
  return 0;
}

// CHECK: define dso_local chericcallcc void @default_enable_callback() local_unnamed_addr addrspace(200) #[[DEFEN]]
__attribute__((cheri_ccallback))
void default_enable_callback(void)
{
}

// CHECK: define dso_local chericcallcc void @default_enable_callback2() local_unnamed_addr addrspace(200) #[[DEFEN]]
__attribute__((cheriot_ccallback))
void default_enable_callback2(void)
{
}


// Explicitly setting interrupt status should override the default

// CHECK: define dso_local chericcallcce i32 @_Z23explicit_disable_calleev() local_unnamed_addr addrspace(200) #[[EXPDIS:[0-9]]]
__attribute__((cheriot_interrupt_state(disabled)))
__attribute__((cheri_compartment("example")))
int explicit_disable_callee(void)
{
  return 0;
}

// CHECK: define dso_local chericcallcc void @explicit_disable_callback() local_unnamed_addr addrspace(200) #[[EXPDIS]]
__attribute__((cheriot_interrupt_state(disabled)))
__attribute__((cheri_ccallback))
void explicit_disable_callback(void)
{
}

// Check deprecated spellings

// CHECK: define dso_local i32 @disabled1() local_unnamed_addr addrspace(200) #[[DIS]]
__attribute__((cheri_interrupt_state(disabled)))
int disabled1(void)
{
	return foo();
}

// CHECK: define dso_local i32 @enabled1() local_unnamed_addr addrspace(200) #[[EN]]
__attribute__((cheri_interrupt_state(enabled)))
int enabled1(void)
{
	return foo();
}

// CHECK: define dso_local i32 @inherit1() local_unnamed_addr addrspace(200) #[[INH]]
__attribute__((cheri_interrupt_state(inherit)))
int inherit1(void)
{
	return foo();
}

// CHECK: define dso_local chericcallcce i32 @_Z24explicit_disable_callee1v() local_unnamed_addr addrspace(200) #[[EXPDIS]]
__attribute__((cheri_interrupt_state(disabled)))
__attribute__((cheri_compartment("example")))
int explicit_disable_callee1(void)
{
  return 0;
}

// CHECK: define dso_local chericcallcc void @explicit_disable_callback1() local_unnamed_addr addrspace(200) #[[EXPDIS]]
__attribute__((cheri_interrupt_state(disabled)))
__attribute__((cheri_ccallback))
void explicit_disable_callback1(void)
{
}

// CHECK: attributes #[[DIS]]
// CHECK-SAME: "interrupt-state"="disabled"
// CHECK: attributes #[[EN]]
// CHECK-SAME: "interrupt-state"="enabled"
// CHECK: attributes #[[INH]]
// CHECK-SAME: "interrupt-state"="inherit"
// CHECK: attributes #[[DEFEN]]
// CHECK-SAME: "interrupt-state"="enabled"
// CHECK: attributes #[[EXPDIS]]
// CHECK-SAME: "interrupt-state"="disabled"
