; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32cheriot-unknown-cheriotrtos -target-abi cheriot -mattr=+xcheri,+cap-mode,+xcheriot %s -o - | FileCheck %s

; Verify that varargs doubles are aligned to 8 bytes on the stack.

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32cheriot-unknown-cheriotrtos"

declare hidden noundef double @_Z7va_testPKcz(ptr addrspace(200) nocapture readnone %c, ...) local_unnamed_addr addrspace(200)

define hidden noundef double @_Z6helperv() local_unnamed_addr addrspace(200) {
; CHECK-LABEL: _Z6helperv:
; CHECK-NOT: csw [.*], 4(csp)
; CHECK-DAG: lui [[R0:.*]], 261939
; CHECK-DAG: lui [[R1:.*]], 209715
; CHECK-DAG: addi [[R0]], [[R0]], 819
; CHECK-DAG: addi [[R1]], [[R1]], 819
; CHECK-DAG: csw [[R0]], 12(csp)
; CHECK-DAG: csw [[R1]], 8(csp)
; CHECK-NOT: csw [.*], 4(csp)
entry:
  %call = tail call noundef double (ptr addrspace(200), ...) @_Z7va_testPKcz(ptr addrspace(200) nonnull poison, i32 noundef 0, double noundef 1.200000e+00)
  ret double %call
}

!llvm.linker.options = !{}
!llvm.module.flags = !{!0, !1, !2, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
!5 = !{!"clang version 20.1.3 (git@github.com:resistor/llvm-project-1.git 592752fee8b25c925a65fb40eeb8a496f1b0ee2c)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"double", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C++ TBAA"}
