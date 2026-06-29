; RUN: llc --filetype=obj --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap -o - | llvm-readelf -a - | FileCheck %s

; CHECK-LABEL: ELF Header:
; CHECK:          Flags: 0x70009, RVC, RVE, cheriabi, capability mode, cheriot

target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"


define void @foo()  addrspace(200) #0 {
entry:
  ret void
}

attributes #0 = { minsize mustprogress nounwind optsize "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+relax,+xcheri,-64bit,-save-restore" }

!llvm.linker.options = !{}
!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 1, !"SmallDataLimit", i32 8}
!3 = !{!"clang version 13.0.0 (ssh://git@github.com/CHERIoT-Platform/llvm-project 42ccdb1bcc7eb0bf8cc8e493850359f828515495)"}
!4 = !{i64 0, i64 4, !5, i64 4, i64 4, !5, i64 8, i64 4, !5}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C++ TBAA"}
!9 = !{i64 0, i64 128, !10}
!10 = !{!7, !7, i64 0}

