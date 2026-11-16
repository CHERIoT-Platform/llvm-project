// Test that the __cheri_tocap/__cheri_fromcap kind of a dependent cast survives
// a PCH round trip. It used to be dropped, so instantiating the template from
// the PCH treated the cast as a plain C cast and reported
// "use __cheri_fromcap to convert between pointers and capabilities".

// RUN: %riscv64_cheri_cc1 -std=c++17 -emit-llvm -o - %s -include %s | FileCheck %s
// RUN: %riscv64_cheri_cc1 -std=c++17 -x c++-header -emit-pch -o %t %s
// RUN: %riscv64_cheri_cc1 -std=c++17 -emit-llvm -o - %s -include-pch %t | FileCheck %s

#ifndef HEADER
#define HEADER

template <typename T> void *__capability to_cap(T *p) {
  return (__cheri_tocap void *__capability)p;
}
template <typename T> T *from_cap(void *__capability c) {
  return (__cheri_fromcap T *)c;
}

#else

// CHECK-LABEL: define {{.*}} @_Z1fPi(
// CHECK-LABEL: define linkonce_odr {{.*}} ptr addrspace(200) @_Z6to_capIiEU12__capabilityPvPT_(
// CHECK: addrspacecast ptr %{{.*}} to ptr addrspace(200)
void *__capability f(int *p) { return to_cap(p); }

// CHECK-LABEL: define {{.*}} @_Z1gU12__capabilityPv(
// CHECK-LABEL: define linkonce_odr {{.*}} ptr @_Z8from_capIiEPT_U12__capabilityPv(
// CHECK: addrspacecast ptr addrspace(200) %{{.*}} to ptr
int *g(void *__capability c) { return from_cap<int>(c); }

#endif
