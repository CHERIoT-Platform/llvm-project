!DO NOT AUTOGEN!
; This used to split the memcpy/memmove in sroa and create broken output
; RUN: opt @PURECAP_HARDFLOAT_ARGS@ -passes=sroa -S %s -o - | FileCheck %s -check-prefix SROA
; RUN: opt @PURECAP_HARDFLOAT_ARGS@ -passes=sroa -S %s -o - | llc @PURECAP_HARDFLOAT_ARGS@ -O2 -verify-machineinstrs - -o - | FileCheck %s -check-prefixes=CHECK,WITH-SROA
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -O2 -verify-machineinstrs %s -o - | FileCheck %s -check-prefixes=CHECK,WITHOUT-SROA
target datalayout = "@PURECAP_DATALAYOUT@"

%struct.addrinfo = type { ptr addrspace(200) }

declare void @llvm.memcpy.p200.p200.iCAPRANGE(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, iCAPRANGE, i1 immarg) addrspace(200)
declare void @llvm.memmove.p200.p200.iCAPRANGE(ptr addrspace(200) nocapture writeonly, ptr addrspace(200) nocapture readonly, iCAPRANGE, i1 immarg) addrspace(200)

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
  call void @llvm.memcpy.p200.p200.iCAPRANGE(ptr addrspace(200) align 16 %retval, ptr addrspace(200) align 1 %0, iCAPRANGE @CAP_BYTES@, i1 false)
  %coerce.dive = getelementptr inbounds %struct.addrinfo, ptr addrspace(200) %retval, i32 0, i32 0
  %1 = load { ptr addrspace(200) }, ptr addrspace(200)%coerce.dive, align 16
  ret { ptr addrspace(200) } %1
}
; The underaligned cap load is expanded to a bounded memcpy() call, not split.
; CHECK-LABEL: do_not_split_cap_memcpy:
@IF-MIPS@; CHECK: clcbi $c12, %capcall20(memcpy)($c1)
@IF-MIPS@; CHECK-DAG: csetbounds $c3, $c{{3|11}}, @CAP_BYTES@
@IF-MIPS@; CHECK-DAG: cjalr $c12, $c17
@IF-MIPS@; CHECK-DAG: daddiu $4, $zero, @CAP_BYTES@
@IF-MIPS@; CHECK: cjr $c17
@IFNOT-MIPS@; CHECK-DAG: csetbounds {{.+}}, @CAP_BYTES@
@IFNOT-MIPS@; CHECK-DAG: li a2, @CAP_BYTES@
@IFNOT-MIPS@; CHECK: ccall memcpy
@IFNOT-MIPS@; CHECK: cret

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
  call void @llvm.memmove.p200.p200.iCAPRANGE(ptr addrspace(200) align 16 %retval, ptr addrspace(200) align 1 %0, iCAPRANGE @CAP_BYTES@, i1 false)
  %coerce.dive = getelementptr inbounds %struct.addrinfo, ptr addrspace(200) %retval, i32 0, i32 0
  %1 = load { ptr addrspace(200) }, ptr addrspace(200) %coerce.dive, align 16
  ret { ptr addrspace(200) } %1
}
; With SROA the memmove is converted to a memcpy to the stack; without it stays a memmove.
; CHECK-LABEL: do_not_split_cap_memmove:
@IF-MIPS@; WITH-SROA: clcbi $c12, %capcall20(memcpy)($c1)
@IF-MIPS@; WITHOUT-SROA: clcbi $c12, %capcall20(memmove)($c1)
@IF-MIPS@; CHECK-DAG: csetbounds $c3, $c{{3|11}}, @CAP_BYTES@
@IF-MIPS@; CHECK-DAG: cjalr $c12, $c17
@IF-MIPS@; CHECK-DAG: daddiu $4, $zero, @CAP_BYTES@
@IF-MIPS@; CHECK: cjr $c17
@IFNOT-MIPS@; CHECK-DAG: csetbounds {{.+}}, @CAP_BYTES@
@IFNOT-MIPS@; CHECK-DAG: li a2, @CAP_BYTES@
@IFNOT-MIPS@; WITH-SROA: ccall memcpy
@IFNOT-MIPS@; WITHOUT-SROA: ccall memmove
@IFNOT-MIPS@; CHECK: cret
