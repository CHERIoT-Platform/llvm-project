; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/memcpy-unaligned-addrinfo.ll
; This used to split the memcpy/memmove in sroa and create broken output
; RUN: opt -mtriple=riscv32 --relocation-model=pic -target-abi il32pc64f -mattr=+xcheri,+xcheripurecap,+f -passes=sroa -S %s -o - | FileCheck %s -check-prefix SROA
; RUN: opt -mtriple=riscv32 --relocation-model=pic -target-abi il32pc64f -mattr=+xcheri,+xcheripurecap,+f -passes=sroa -S %s -o - | llc -mtriple=riscv32 --relocation-model=pic -target-abi il32pc64f -mattr=+xcheri,+xcheripurecap,+f -O2 -verify-machineinstrs - -o - | FileCheck %s -check-prefixes=CHECK,WITH-SROA
; RUN: llc -mtriple=riscv32 --relocation-model=pic -target-abi il32pc64f -mattr=+xcheri,+xcheripurecap,+f -O2 -verify-machineinstrs %s -o - | FileCheck %s -check-prefixes=CHECK,WITHOUT-SROA
target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"

%struct.addrinfo = type { ptr addrspace(200) }

declare void @llvm.memcpy.p200.p200.i32(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i32, i1 immarg) addrspace(200)
declare void @llvm.memmove.p200.p200.i32(ptr addrspace(200) nocapture writeonly, ptr addrspace(200) nocapture readonly, i32, i1 immarg) addrspace(200)

define inreg { ptr addrspace(200) } @do_not_split_cap_memcpy(ptr addrspace(200) %a) addrspace(200) noinline nounwind {
; The memcpy is not split: SROA replaces it with a single underaligned cap load.
; SROA-LABEL: @do_not_split_cap_memcpy(
; SROA-NEXT: entry:
; SROA-NEXT: [[COPYLOAD:%.*]] = load ptr addrspace(200), ptr addrspace(200) %a, align 1
; SROA-NEXT: [[INSERT:%.*]] = insertvalue { ptr addrspace(200) } poison, ptr addrspace(200) [[COPYLOAD]], 0
; SROA-NEXT: ret { ptr addrspace(200) } [[INSERT]]
entry:
  %retval = alloca %struct.addrinfo, align 16, addrspace(200)
  %a.addr = alloca ptr addrspace(200), align 16, addrspace(200)
  store ptr addrspace(200) %a, ptr addrspace(200) %a.addr, align 16
  %0 = load ptr addrspace(200), ptr addrspace(200) %a.addr, align 16
  call void @llvm.memcpy.p200.p200.i32(ptr addrspace(200) align 16 %retval, ptr addrspace(200) align 1 %0, i32 8, i1 false)
  %coerce.dive = getelementptr inbounds %struct.addrinfo, ptr addrspace(200) %retval, i32 0, i32 0
  %1 = load { ptr addrspace(200) }, ptr addrspace(200)%coerce.dive, align 16
  ret { ptr addrspace(200) } %1
}
; The underaligned cap load is expanded to a bounded memcpy() call, not split.
; CHECK-LABEL: do_not_split_cap_memcpy:
; CHECK-DAG: csetbounds {{.+}}, 8
; CHECK-DAG: li a2, 8
; CHECK: ccall memcpy
; CHECK: cret

define inreg { ptr addrspace(200) } @do_not_split_cap_memmove(ptr addrspace(200) %a) addrspace(200) noinline nounwind {
; SROA-LABEL: @do_not_split_cap_memmove(
; SROA-NEXT: entry:
; SROA-NEXT: [[COPYLOAD:%.*]] = load ptr addrspace(200), ptr addrspace(200) %a, align 1
; SROA-NEXT: [[INSERT:%.*]] = insertvalue { ptr addrspace(200) } poison, ptr addrspace(200) [[COPYLOAD]], 0
; SROA-NEXT: ret { ptr addrspace(200) } [[INSERT]]
entry:
  %retval = alloca %struct.addrinfo, align 16, addrspace(200)
  %a.addr = alloca ptr addrspace(200), align 16, addrspace(200)
  store ptr addrspace(200) %a, ptr addrspace(200) %a.addr, align 16
  %0 = load ptr addrspace(200), ptr addrspace(200) %a.addr, align 16
  call void @llvm.memmove.p200.p200.i32(ptr addrspace(200) align 16 %retval, ptr addrspace(200) align 1 %0, i32 8, i1 false)
  %coerce.dive = getelementptr inbounds %struct.addrinfo, ptr addrspace(200) %retval, i32 0, i32 0
  %1 = load { ptr addrspace(200) }, ptr addrspace(200) %coerce.dive, align 16
  ret { ptr addrspace(200) } %1
}
; With SROA the memmove is converted to a memcpy to the stack; without it stays a memmove.
; CHECK-LABEL: do_not_split_cap_memmove:
; CHECK-DAG: csetbounds {{.+}}, 8
; CHECK-DAG: li a2, 8
; WITH-SROA: ccall memcpy
; WITHOUT-SROA: ccall memmove
; CHECK: cret
