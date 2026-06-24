; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/load-store-unaligned.ll
; Check that expanding unaligned capability loads and stores works (but generates a warning)
; RUN: rm -f %t.dbg
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi purecap -verify-machineinstrs %s -o /dev/null -collect-csetbounds-stats=csv 2>%t.dbg
; RUN: FileCheck %s -input-file=%t.dbg -check-prefix=DBG

define ptr addrspace(200) @load_unaligned(ptr addrspace(200) %unaligned) local_unnamed_addr addrspace(200) nounwind {
entry:
  %r.0..sroa_cast = bitcast ptr addrspace(200) %unaligned to ptr addrspace(200)
  %r.0.copyload = load ptr addrspace(200), ptr addrspace(200) %r.0..sroa_cast, align 4
  ret ptr addrspace(200) %r.0.copyload
}

define void @store_unaligned(ptr addrspace(200) %unused, ptr addrspace(200) %unaligned, ptr addrspace(200) %value) local_unnamed_addr addrspace(200) nounwind {
entry:
  %r.0..sroa_cast = bitcast ptr addrspace(200) %unaligned to ptr addrspace(200)
  store ptr addrspace(200) %value, ptr addrspace(200) %r.0..sroa_cast, align 4
  ret void
}

; This should be turned into a memmove:
define void @store_of_unaligned_load(ptr addrspace(200) %src, ptr addrspace(200) %dest, ptr addrspace(200) %value) local_unnamed_addr addrspace(200) nounwind {
entry:
  %src_cast = bitcast ptr addrspace(200) %src to ptr addrspace(200)
  %dest_cast = bitcast ptr addrspace(200) %dest to ptr addrspace(200)
  %unaligned_load = load ptr addrspace(200), ptr addrspace(200) %src_cast, align 4
  store ptr addrspace(200) %unaligned_load, ptr addrspace(200) %dest_cast, align 8
  ret void
}

declare ptr addrspace(200) @llvm.cheri.cap.offset.set.i64(ptr addrspace(200), i64) addrspace(200)

declare i64 @llvm.cheri.cap.offset.get.i64(ptr addrspace(200)) addrspace(200)

; DBG: warning: <unknown>:0:0: in function load_unaligned ptr addrspace(200) (ptr addrspace(200)): found underaligned load of capability type (aligned to 4 bytes instead of 16). Will use memcpy() instead of capability load to preserve tags if it is aligned correctly at runtime
; DBG-NEXT: warning: <unknown>:0:0: in function store_unaligned void (ptr addrspace(200), ptr addrspace(200), ptr addrspace(200)): found underaligned store of capability type (aligned to 4 bytes instead of 16). Will use memcpy() instead of capability load to preserve tags if it is aligned correctly at runtime
; DBG-NEXT: warning: <unknown>:0:0: in function store_of_unaligned_load void (ptr addrspace(200), ptr addrspace(200), ptr addrspace(200)): found underaligned store of underaligned load of capability type (aligned to 8 bytes instead of 16). Will use memmove() to preserve tags if it is aligned correctly at runtime
; DBG-NEXT: 4,16,s,"<somewhere in load_unaligned>","expanding unaligned capability load/store","expanding unaligned capability load stack destination"
; DBG-NEXT: 4,16,s,"<somewhere in load_unaligned>","expanding unaligned capability load/store","expanding unaligned capability load memcpy source"
; DBG-NEXT: 4,16,s,"<somewhere in store_unaligned>","expanding unaligned capability load/store","expanding unaligned capability store stack source"
; DBG-NEXT: 4,16,s,"<somewhere in store_unaligned>","expanding unaligned capability load/store","expanding unaligned capability store memcpy destination"
; DBG-NEXT: 4,16,s,"<somewhere in store_of_unaligned_load>","expanding unaligned capability load/store","expanding unaligned capability store+load memmove src"
; DBG-NEXT: 4,16,s,"<somewhere in store_of_unaligned_load>","expanding unaligned capability load/store","expanding unaligned capability store+load memmove dest"
; DBG-EMPTY:
