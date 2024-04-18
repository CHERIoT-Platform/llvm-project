; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot  %s -mattr=+xcheri,+cap-mode -o - | FileCheck %s
; ModuleID = 'stack.c'
source_filename = "stack.c"
target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-unknown"

; Explicit 256-byte stack requirement
; Function Attrs: minsize nounwind optsize
define dso_local chericcallcce i32 @needs256() local_unnamed_addr addrspace(200) #0 {
entry:
  %call = tail call i32 @foo() #2
  ret i32 %call
}

; Function Attrs: minsize optsize
declare i32 @foo() local_unnamed_addr addrspace(200) #1

; Explicit 512-byte stack requirement
; Function Attrs: minsize nounwind optsize
define dso_local chericcallcce i32 @needs512() local_unnamed_addr addrspace(200) #3 {
entry:
  %call = tail call i32 @foo() #2
  ret i32 %call
}

; No stack requirement
; Function Attrs: minsize nounwind optsize
define dso_local chericcallcce i32 @noStack() local_unnamed_addr addrspace(200) #4 {
entry:
  ret i32 0
}

; Implicit 16-byte requirement as a result of spilling the return value (and
; the 16-byte alignment requirement)
; Function Attrs: minsize nounwind optsize
define dso_local chericcallcce i32 @tinyStack() local_unnamed_addr addrspace(200) #4 {
entry:
  %call = tail call i32 @foo() #2
  %add1 = add nsw i32 %call, 1
  ret i32 %add1
}



attributes #0 = { minsize nounwind optsize "cheri-compartment"="example" "frame-pointer"="none" "interrupt-state"="enabled" "min-legal-vector-width"="0" "minimum-stack-size"="256" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+xcheri,-64bit,-relax,-save-restore,-xcheri-rvc" }
attributes #1 = { minsize optsize "cheri-compartment"="example" "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+xcheri,-64bit,-relax,-save-restore,-xcheri-rvc" }
attributes #2 = { minsize nounwind optsize }
attributes #3 = { minsize nounwind optsize "cheri-compartment"="example" "frame-pointer"="none" "interrupt-state"="enabled" "min-legal-vector-width"="0" "minimum-stack-size"="512" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+xcheri,-64bit,-relax,-save-restore,-xcheri-rvc" }
attributes #4 = { minsize nounwind optsize "cheri-compartment"="example" "frame-pointer"="none" "interrupt-state"="enabled" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+xcheri,-64bit,-relax,-save-restore,-xcheri-rvc" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 1, !"Code Model", i32 1}
!3 = !{i32 1, !"SmallDataLimit", i32 0}
!4 = !{!"clang version 13.0.0 (ssh://git@github.com/CHERIoT-Platform/llvm-project 71e6c1e2702c542a1581a2b7da0d62256bbba667)"}


; CHECK: __export_example_needs256:
; CHECK-NEXT: .half	needs256-__compartment_pcc_start
; CHECK-NEXT: .byte	32
; CHECK: __export_example_needs512:
; CHECK-NEXT: .half	needs512-__compartment_pcc_start
; CHECK-NEXT: .byte	64
; CHECK: __export_example_noStack:
; CHECK-NEXT: .half	noStack-__compartment_pcc_start
; CHECK-NEXT: .byte	0
; CHECK: __export_example_tinyStack:
; CHECK-NEXT: .half	tinyStack-__compartment_pcc_start
; CHECK-NEXT: .byte	2
