// RUN: %clang_cc1 -triple riscv32-unknown-cheriotrtos -target-abi cheriot \
// RUN:   -target-feature +xcheriot -emit-llvm -o - %s | FileCheck %s

// *(p + i) with an __intcap_t index used to crash.

// CHECK-LABEL: define {{.*}} @f(
// CHECK: [[ADDR:%.*]] = call {{.*}}i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200) %{{.*}})
// CHECK: getelementptr inbounds i32, ptr addrspace(200) %{{.*}}, i32 [[ADDR]]
int f(int *p, __intcap_t i) { return *(p + i); }

// CHECK-LABEL: define {{.*}} @g(
// CHECK: [[ADDR:%.*]] = call {{.*}}i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200) %{{.*}})
// CHECK: getelementptr inbounds i32, ptr addrspace(200) %{{.*}}, i32 [[ADDR]]
int g(int *p, __intcap_t i) { return *(i + p); }
