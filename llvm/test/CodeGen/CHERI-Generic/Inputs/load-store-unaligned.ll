; !DO NOT AUTOGEN!
; Check that expanding unaligned capability loads and stores works (but generates a warning)
; RUN: rm -f %t.dbg
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o /dev/null -collect-csetbounds-stats=csv 2>%t.dbg
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
  store ptr addrspace(200) %value, ptr addrspace(200) %r.0..sroa_cast, align 4, !dbg !11
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

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !8, isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly, enums: !2, nameTableKind: None)
!1 = !DIFile(filename: "sroa-libunwind.cxx", directory: "/some/dir")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 5}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!7 = distinct !DISubprogram(name: "store_unaligned", scope: !8, file: !8, line: 7, scopeLine: 7, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !2)
!8 = !DIFile(filename: "sroa-libunwind.cxx", directory: "/foo")
!11 = !DILocation(line: 9, column: 3, scope: !7)

; DBG: warning: <unknown>:0:0: in function load_unaligned ptr addrspace(200) (ptr addrspace(200)): found underaligned load of capability type (aligned to 4 bytes instead of @CAP_BYTES@). Will use memcpy() instead of capability load to preserve tags if it is aligned correctly at runtime
; DBG-NEXT: warning: sroa-libunwind.cxx:9:3: in function store_unaligned void (ptr addrspace(200), ptr addrspace(200), ptr addrspace(200)): found underaligned store of capability type (aligned to 4 bytes instead of @CAP_BYTES@). Will use memcpy() instead of capability load to preserve tags if it is aligned correctly at runtime
@IFNOT-RISCV32@; DBG-NEXT: warning: <unknown>:0:0: in function store_of_unaligned_load void (ptr addrspace(200), ptr addrspace(200), ptr addrspace(200)): found underaligned store of underaligned load of capability type (aligned to 8 bytes instead of @CAP_BYTES@). Will use memmove() to preserve tags if it is aligned correctly at runtime
@IF-RISCV32@; DBG-NEXT: warning: <unknown>:0:0: in function store_of_unaligned_load void (ptr addrspace(200), ptr addrspace(200), ptr addrspace(200)): found underaligned load of capability type (aligned to 4 bytes instead of @CAP_BYTES@). Will use memcpy() instead of capability load to preserve tags if it is aligned correctly at runtime
; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"<somewhere in load_unaligned>","expanding unaligned capability load/store","expanding unaligned capability load stack destination"
; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"<somewhere in load_unaligned>","expanding unaligned capability load/store","expanding unaligned capability load memcpy source"
; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"sroa-libunwind.cxx:9:3","expanding unaligned capability load/store","expanding unaligned capability store stack source"
; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"sroa-libunwind.cxx:9:3","expanding unaligned capability load/store","expanding unaligned capability store memcpy destination"
@IFNOT-RISCV32@; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"<somewhere in store_of_unaligned_load>","expanding unaligned capability load/store","expanding unaligned capability store+load memmove src"
@IFNOT-RISCV32@; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"<somewhere in store_of_unaligned_load>","expanding unaligned capability load/store","expanding unaligned capability store+load memmove dest"
@IF-RISCV32@; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"<somewhere in store_of_unaligned_load>","expanding unaligned capability load/store","expanding unaligned capability load stack destination"
@IF-RISCV32@; DBG-NEXT: @CAP_BYTES_P2@,@CAP_BYTES@,s,"<somewhere in store_of_unaligned_load>","expanding unaligned capability load/store","expanding unaligned capability load memcpy source"
; DBG-EMPTY:
