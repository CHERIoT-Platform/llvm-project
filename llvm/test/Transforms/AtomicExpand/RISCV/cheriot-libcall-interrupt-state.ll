; RUN: opt -S -mtriple=riscv32-unknown-cheriotrtos -mcpu=cheriot -target-abi cheriot \
; RUN:   -mattr=+xcheri,+xcheripurecap \
; RUN:   -passes='require<libcall-lowering-info>,atomic-expand' %s | FileCheck %s --check-prefix=CHERIOT
; RUN: opt -S -mtriple=riscv32-unknown-unknown -target-abi il32pc64 \
; RUN:   -mattr=+xcheri,+xcheripurecap \
; RUN:   -passes='require<libcall-lowering-info>,atomic-expand' %s | FileCheck %s --check-prefix=PURECAP

; On CHERIoT, the __atomic_* libcalls must run with interrupts disabled, so
; every call and declaration produced by AtomicExpand carries
; "interrupt-state"="disabled". This covers the sized, generic, fetch and
; capability (_cap) variants. Other CHERI targets must not get the attribute.

target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"

define i32 @load_sized(ptr addrspace(200) %p) {
; CHERIOT-LABEL: @load_sized(
; CHERIOT: call cheriot_librarycallcc addrspace(200) i32 @__atomic_load_4(ptr addrspace(200) %p, i32 5) [[ATTR:#[0-9]+]]
; PURECAP-LABEL: @load_sized(
; PURECAP: call {{.*}}@__atomic_load_4(ptr addrspace(200) %p, i32 5){{$}}
  %v = load atomic i32, ptr addrspace(200) %p seq_cst, align 4
  ret i32 %v
}

define void @store_sized(ptr addrspace(200) %p, i32 %v) {
; CHERIOT-LABEL: @store_sized(
; CHERIOT: call cheriot_librarycallcc addrspace(200) void @__atomic_store_4(ptr addrspace(200) %p, i32 %v, i32 5) [[ATTR]]
; PURECAP-LABEL: @store_sized(
; PURECAP: call {{.*}}@__atomic_store_4(ptr addrspace(200) %p, i32 %v, i32 5){{$}}
  store atomic i32 %v, ptr addrspace(200) %p seq_cst, align 4
  ret void
}

define i32 @fetch_add(ptr addrspace(200) %p, i32 %v) {
; CHERIOT-LABEL: @fetch_add(
; CHERIOT: call cheriot_librarycallcc addrspace(200) i32 @__atomic_fetch_add_4(ptr addrspace(200) %p, i32 %v, i32 5) [[ATTR]]
; PURECAP-LABEL: @fetch_add(
; PURECAP: call {{.*}}@__atomic_fetch_add_4(ptr addrspace(200) %p, i32 %v, i32 5){{$}}
  %r = atomicrmw add ptr addrspace(200) %p, i32 %v seq_cst
  ret i32 %r
}

define i64 @load_generic(ptr addrspace(200) %p) {
; CHERIOT-LABEL: @load_generic(
; CHERIOT: call cheriot_librarycallcc addrspace(200) void @__atomic_load(i32 8, ptr addrspace(200) %p, ptr addrspace(200) %{{.*}}, i32 5) [[ATTR]]
; PURECAP-LABEL: @load_generic(
; PURECAP: call {{.*}}@__atomic_load(i32 8, ptr addrspace(200) %p, ptr addrspace(200) %{{.*}}, i32 5){{$}}
  %v = load atomic i64, ptr addrspace(200) %p seq_cst, align 4
  ret i64 %v
}

define ptr addrspace(200) @exchange_cap(ptr addrspace(200) %p, ptr addrspace(200) %c) {
; CHERIOT-LABEL: @exchange_cap(
; CHERIOT: call cheriot_librarycallcc addrspace(200) ptr addrspace(200) @__atomic_exchange_cap(ptr addrspace(200) %p, ptr addrspace(200) %c, i32 5) [[ATTR]]
; PURECAP-LABEL: @exchange_cap(
; PURECAP: call {{.*}}@__atomic_exchange_cap(ptr addrspace(200) %p, ptr addrspace(200) %c, i32 5){{$}}
  %r = atomicrmw xchg ptr addrspace(200) %p, ptr addrspace(200) %c seq_cst
  ret ptr addrspace(200) %r
}

define { ptr addrspace(200), i1 } @cmpxchg_cap(ptr addrspace(200) %p, ptr addrspace(200) %e, ptr addrspace(200) %n) {
; CHERIOT-LABEL: @cmpxchg_cap(
; CHERIOT: call cheriot_librarycallcc zeroext addrspace(200) i1 @__atomic_compare_exchange_cap(ptr addrspace(200) %p, ptr addrspace(200) %{{.*}}, ptr addrspace(200) %n, i32 5, i32 5) [[ATTR]]
; PURECAP-LABEL: @cmpxchg_cap(
; PURECAP: call {{.*}}@__atomic_compare_exchange_cap(ptr addrspace(200) %p, ptr addrspace(200) %{{.*}}, ptr addrspace(200) %n, i32 5, i32 5){{$}}
  %r = cmpxchg ptr addrspace(200) %p, ptr addrspace(200) %e, ptr addrspace(200) %n seq_cst seq_cst
  ret { ptr addrspace(200), i1 } %r
}

; CHERIOT-DAG: declare cheriot_librarycallcc i32 @__atomic_load_4(ptr addrspace(200), i32) addrspace(200) [[ATTR]]
; CHERIOT-DAG: declare cheriot_librarycallcc void @__atomic_store_4(ptr addrspace(200), i32, i32) addrspace(200) [[ATTR]]
; CHERIOT-DAG: declare cheriot_librarycallcc i32 @__atomic_fetch_add_4(ptr addrspace(200), i32, i32) addrspace(200) [[ATTR]]
; CHERIOT-DAG: declare cheriot_librarycallcc void @__atomic_load(i32, ptr addrspace(200), ptr addrspace(200), i32) addrspace(200) [[ATTR]]
; CHERIOT-DAG: declare cheriot_librarycallcc ptr addrspace(200) @__atomic_exchange_cap(ptr addrspace(200), ptr addrspace(200), i32) addrspace(200) [[ATTR]]
; CHERIOT-DAG: declare cheriot_librarycallcc zeroext i1 @__atomic_compare_exchange_cap(ptr addrspace(200), ptr addrspace(200), ptr addrspace(200), i32, i32) addrspace(200) [[ATTR]]
; CHERIOT: attributes [[ATTR]] = { "interrupt-state"="disabled" }

; PURECAP-NOT: interrupt-state
