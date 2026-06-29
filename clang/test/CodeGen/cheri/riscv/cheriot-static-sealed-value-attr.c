// RUN: %clang_cc1 %s -o - "-triple" "riscv32-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" "-target-feature" "+xcheriot" -std=c2x | FileCheck %s

// Test that sealed variables behave like we expect them to.
// In this test we have two global variables that refer to sealed values.

// CHECK: %struct.__Sealed_const_int = type { i32, i32, i32 }
// CHECK: %struct.__Sealed_int = type { i32, i32, i32 }

// CHECK: $MyHereInt = comdat any
// CHECK: @MyHereInt = linkonce_odr dso_local addrspace(200) constant %struct.__Sealed_const_int { i32 ptrtoint (ptr addrspace(200) @__export.sealing_type.MyCompartment.MyKeyName to i32), i32 0, i32 0 }, section ".sealed_objects", comdat, align 4 #0
const int __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) MyHereInt = 0;

// CHECK: @__export.sealing_type.MyCompartment.MyKeyName = external dso_local addrspace(200) global i32, align 4

// CHECK: @MyExternInt = external addrspace(200) global %struct.__Sealed_int, align 4 #0
extern int __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) MyExternInt;

// Mark `MyHereInt` as used, so it can't be optimised away.
// CHECK: @llvm.compiler.used = appending addrspace(200) global [1 x ptr addrspace(200)] [ptr addrspace(200) @MyHereInt], section "llvm.metadata"

void doSomethingWithSealed(const int* __sealed_capability);
void doSomethingWithAddr(int addr);

void func() {

  // CHECK: tail call addrspace(200) void @doSomethingWithSealed(ptr addrspace(200) noundef nonnull @MyHereInt) #5
  doSomethingWithSealed(&MyHereInt);

  // CHECK: tail call addrspace(200) void @doSomethingWithSealed(ptr addrspace(200) noundef nonnull @MyExternInt) #5
  doSomethingWithSealed(&MyExternInt);

  // Verify that observing the address of a sealed global value is done through a call to the launder built-in, so that 
  // the KnownBits optimisation pass can't assume anything about the value of the pointer, specifically nothing about the alignment of the pointer.
  // This is because the CHERIoT RTOS uses the lower bits of the address to store the permissions of the sealed capability, and KnownBits can in turn 
  // optimise away logical computations on lower parts of the address.
  
  // CHECK: %0 = tail call addrspace(200) ptr addrspace(200) @llvm.launder.alignment.p200(ptr addrspace(200) nonnull @MyHereInt)
  // CHECK: %1 = tail call addrspace(200) i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200) nonnull %0)
  // CHECK: tail call addrspace(200) void @doSomethingWithAddr(i32 noundef %1) #5
  doSomethingWithAddr(__builtin_cheri_address_get(&MyHereInt));

  /// Same with an extern declaration.
  // CHECK: %2 = tail call addrspace(200) ptr addrspace(200) @llvm.launder.alignment.p200(ptr addrspace(200) nonnull @MyExternInt)
  // CHECK: %3 = tail call addrspace(200) i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200) nonnull %2)
  // CHECK: tail call addrspace(200) void @doSomethingWithAddr(i32 noundef %3) #5
  doSomethingWithAddr(__builtin_cheri_address_get(&MyExternInt));

  // CHECK: ret void
}

// CHECK:  attributes #0 = { "cheriot_sealed_value" }
