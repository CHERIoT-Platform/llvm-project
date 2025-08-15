; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap -o - | FileCheck %s
source_filename = "/usr/home/theraven/llvm-project/clang/test/CodeGen/cheri/cheri-mcu-call-libcall.c"
target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-unknown"

; Function Attrs: minsize nounwind optsize
; CHECK-LABEL: callFromNotLibcall:
define dso_local i32 @callFromNotLibcall() local_unnamed_addr addrspace(200) #0 {
entry:
; Check that these are direct calls via the import table, not going via the
; compartment switcher.
; CHECK: li    a0, 1
; CHECK: li    a1, 2
; CHECK: auipcc  ct2, %cheriot_compartment_hi(__library_import_libcalls_add)
; CHECK: clc     ct2, %cheriot_compartment_lo_i(.LBB0_1)(ct2)
; CHECK: cjalr   ct2
  %call = tail call cherilibcallcc i32 @add(i32 1, i32 2) #2
; CHECK: auipcc  ct2, %cheriot_compartment_hi(__library_import_libcalls_foo)
; CHECK: clc     ct2, %cheriot_compartment_lo_i(.LBB0_2)(ct2)
; CHECK: cjalr   ct2
  %call1 = tail call cherilibcallcc i32 @foo() #2
  %add = add nsw i32 %call1, %call
  ret i32 %add
}

; CHECK-LABEL: callViaAlias:
define dso_local i32 @callViaAlias() local_unnamed_addr addrspace(200) #0 {
entry:
; CHECK: ccall bar_alias
  %call1 = tail call cherilibcallcc i32 @bar_alias() #2
  ret i32 %call1
}

define cherilibcallcc i32 @bar() local_unnamed_addr addrspace(200) #0 {
entry:
  ret i32 0
}

; Function Attrs: minsize optsize
declare cherilibcallcc i32 @add(i32, i32) local_unnamed_addr addrspace(200) #1

; Function Attrs: minsize optsize
declare cherilibcallcc i32 @foo() local_unnamed_addr addrspace(200) #1

; CHECK: .set bar_alias, bar
@bar_alias = weak_odr dso_local unnamed_addr alias void (), ptr addrspace(200) @bar

attributes #0 = { minsize nounwind optsize "cheri-compartment"="foo" "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+xcheri,-64bit,-relax,-save-restore,-no-rvc-hints" }
attributes #1 = { minsize optsize "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+xcheri,-64bit,-relax,-save-restore,+no-rvc-hints" }
attributes #2 = { minsize nounwind optsize }

!llvm.module.flags = !{!0, !1, !2, !3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 1, !"Code Model", i32 1}
!3 = !{i32 1, !"SmallDataLimit", i32 0}

; Check that the low bit is set in the import table entries.
; CHECK: __library_import_libcalls_add:
; CHECK:         .word   __library_export_libcalls_add+1
; CHECK:         .word   0
; CHECK:         .size   __library_import_libcalls_add, 8
; CHECK: __library_import_libcalls_foo:
; CHECK:         .word   __library_export_libcalls_foo+1
