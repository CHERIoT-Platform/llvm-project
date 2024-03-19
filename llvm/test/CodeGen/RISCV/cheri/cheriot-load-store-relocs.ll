; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot  %s -mattr=+xcheri,+cap-mode -o - | FileCheck %s
; ModuleID = 'ex.c'
target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-unknown"

@temp = dso_local local_unnamed_addr addrspace(200) global i32 0, align 4
@bigArray = internal addrspace(200) global [1025 x i32] zeroinitializer, align 4


; Function Attrs: minsize mustprogress nofree norecurse nosync nounwind optsize willreturn
define dso_local i32 @example_entry(i32 addrspace(200)* nocapture readnone %input) local_unnamed_addr addrspace(200) #0 {
entry:
  ; Check that we generate relocations folded into the loads and stores for
  ; globals.  This is currently broken by the move to post-RA expansion of
  ; cllc.  Keep the test around to check for the new sequence.  If the
  ; optimisation is reintroduced, delete the X from the XCHECK lines.
  ; CHECK: auicgp
  ; CHECK-SAME: %cheriot_compartment_hi(temp)
  ; XCHECK: clw
  ; CHECK: cincoffset
  ; CHECK-SAME: %cheriot_compartment_lo_i
  %0 = load i32, i32 addrspace(200)* @temp, align 4, !tbaa !5
  %inc = add nsw i32 %0, 1
  store i32 %inc, i32 addrspace(200)* @temp, align 4, !tbaa !5
  ret i32 %0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind readnone willreturn
define dso_local i32 addrspace(200)* @getBigArray() local_unnamed_addr addrspace(200) #0 {
entry:
  ; CHECK-LABEL: getBigArray:
  ; Check that we're materialising the constant with two instructions
  ; CHECK: lui [[BOUNDS_HI:[a-z]+[0-9]+]], 1
  ; CHECK: addi [[BOUNDS:[a-z]+[0-9]+]], [[BOUNDS_HI]], 4
  ; Check that we're using the correct relocation for the global
  ; CHECK: auicgp
  ; CHECK-SAME: %cheriot_compartment_hi(bigArray)
  ; CHECK: cincoffset
  ; CHECK-SAME: %cheriot_compartment_lo_i
  ; Slightly hacky pattern to make sure that we're emitting the using the value
  ; that we calculated, rather than an immediate or relocation.
  ; CHECK: csetbounds	c{{.[0-9]+}}, c{{.[0-9]+}}, [[BOUNDS]]

  ret i32 addrspace(200)* getelementptr inbounds ([1025 x i32], [1025 x i32] addrspace(200)* @bigArray, i32 0, i32 0)
}


attributes #0 = { minsize mustprogress nofree norecurse nosync nounwind optsize willreturn "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+xcheri,-64bit,-relax,-save-restore,-xcheri-rvc" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 1, !"Code Model", i32 0}
!3 = !{i32 1, !"SmallDataLimit", i32 8}
!4 = !{!"clang version 13.0.0 (git@ssh.dev.azure.com:v3/Portmeirion/CHERI-MCU/LLVM 18e025b950174e35f2345e10f667bb442611895b)"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
