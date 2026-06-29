; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-cheriotrtos -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap -o - | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; Function Attrs: minsize mustprogress nofree norecurse nosync nounwind optsize willreturn memory(none)
define dso_local noundef i32 @test() local_unnamed_addr addrspace(200) #0 {
entry:
  ret i32 0
}

; CHECK:      .section ".note.cheriot.compartment-name","G",@note,".note.cheriot.compartment-name",comdat
; CHECK-NEXT: .ascii   "example"

attributes #0 = { minsize mustprogress nofree norecurse nosync nounwind optsize willreturn memory(none) "cheri-compartment"="example" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+e,+m,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul" }

!llvm.module.flags = !{!0, !1, !2, !4, !5, !6}
!llvm.ident = !{!7}
!llvm.errno.tbaa = !{!8}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_zca1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"}
!4 = !{i32 1, !"cheriot-compartment", !"example"}
!5 = !{i32 1, !"Code Model", i32 1}
!6 = !{i32 8, !"SmallDataLimit", i32 0}
!7 = !{!"clang version 22.1.5 (git@github.com:resistor/llvm-project-1.git f6a1713e2c306058814425775c003bc86740583f)"}
!8 = !{!9, !9, i64 0}
!9 = !{!"int", !10, i64 0}
!10 = !{!"omnipotent char", !11, i64 0}
!11 = !{!"Simple C/C++ TBAA"}
