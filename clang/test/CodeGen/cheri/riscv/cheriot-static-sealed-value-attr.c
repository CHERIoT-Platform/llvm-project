// RUN: %clang_cc1 %s -o - "-triple" "riscv32cheriot-unknown-cheriotrtos" "-emit-llvm" "-mframe-pointer=none" "-mcmodel=small" "-target-abi" "cheriot" "-Oz" "-Werror" "-target-feature" "+xcheriot" -std=c2x | FileCheck %s

// CHECK: %struct.__Sealed_SealedStructObj = type { i32, i32, %struct.SealedStructObj }
// CHECK: %struct.SealedStructObj = type { i32 }
struct __attribute__((cheriot_sealed("MyCompartment", "MyKeyName"))) SealedStructObj { int val; };

// CHECK: %struct.__Sealed_SealedInt = type { i32, i32, i32 }
typedef int SealedInt __attribute((cheriot_sealed("MyCompartment", "MyKeyName")));

// Objects are marked as used. 
//
// CHECK: $Obj1 = comdat any
// CHECK: $Obj2 = comdat any
//
// There's only an exported sealing key for each combination.
//
// CHECK: @__export.sealing_type.MyCompartment.MyKeyName = external dso_local addrspace(200) global i32, align 4
//
// CHECK: @Obj1 = linkonce_odr dso_local addrspace(200) global %struct.__Sealed_SealedStructObj { i32 ptrtoint (ptr addrspace(200) @__export.sealing_type.MyCompartment.MyKeyName to i32), i32 0, %struct.SealedStructObj { i32 10 } }, section ".sealed_objects", comdat, align 4 #0
struct SealedStructObj Obj1 = {10};

// CHECK: @Obj2 = linkonce_odr dso_local addrspace(200) global %struct.__Sealed_SealedInt { i32 ptrtoint (ptr addrspace(200) @__export.sealing_type.MyCompartment.MyKeyName to i32), i32 0, i32 10 }, section ".sealed_objects", comdat, align 4 #0
SealedInt Obj2 = 10;

// CHECK: @llvm.compiler.used = appending addrspace(200) global [2 x ptr addrspace(200)] [ptr addrspace(200) @Obj1, ptr addrspace(200) @Obj2], section "llvm.metadata"

void doSomething(struct SealedStructObj *__sealed_capability obj);
void doSomethingWithAddr(int addr);
void doSomething2(SealedInt *__sealed_capability obj);

// CHECK: ; Function Attrs: minsize nounwind optsize
// CHECK: define dso_local void @func() local_unnamed_addr addrspace(200) #1 {
void func() {
// CHECK: entry:
// CHECK: 	tail call addrspace(200) void @doSomething(ptr addrspace(200) noundef nonnull @Obj1) #5
  doSomething(&Obj1);

// Verify that observing the address of a sealed global value is done through a call to the launder built-in, so that 
// the KnownBits optimisation pass can't assume anything about the value of the pointer, specifically nothing about the alignment of the pointer.
// This is because the CHERIoT RTOS uses the lower bits of the address to store the permissions of the sealed capability, and KnownBits can in turn 
// optimise away logical computations on lower parts of the address.

// CHECK:  %0 = tail call addrspace(200) ptr addrspace(200) @llvm.launder.alignment.p200(ptr addrspace(200) nonnull @Obj1)
// CHECK:  %1 = tail call addrspace(200) i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200) nonnull %0)
// CHECK:  tail call addrspace(200) void @doSomethingWithAddr(i32 noundef %1) #5
  doSomethingWithAddr(__builtin_cheri_address_get(&Obj1));

// CHECK: 	tail call addrspace(200) void @doSomething2(ptr addrspace(200) noundef nonnull @Obj2) #5
  doSomething2(&Obj2);
}

// CHECK: declare void @doSomething(ptr addrspace(200) noundef) local_unnamed_addr addrspace(200) #2
// CHECK: declare void @doSomethingWithAddr(i32 noundef) local_unnamed_addr addrspace(200) #2
// CHECK: declare ptr addrspace(200) @llvm.launder.alignment.p200(ptr addrspace(200)) addrspace(200) #3
// CHECK: declare i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200)) addrspace(200) #4
// CHECK: declare void @doSomething2(ptr addrspace(200) noundef) local_unnamed_addr addrspace(200) #2
