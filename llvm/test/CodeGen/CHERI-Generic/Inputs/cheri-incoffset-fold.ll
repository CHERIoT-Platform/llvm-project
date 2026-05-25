; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -O1 %s -o - | FileCheck %s --check-prefix=PURECAP

; Function Attrs: norecurse nounwind
define void @doThing(ptr addrspace(200) nocapture readonly %in, ptr addrspace(200) nocapture %out) local_unnamed_addr #0 {
entry:
  ; Check that the +8 is folded into the load / store and isn't a separate cincoffset
  %arrayidx = getelementptr inbounds i8, ptr addrspace(200) %in, i64 8
  %0 = load i8, ptr addrspace(200) %arrayidx, align 1, !tbaa !3
  %arrayidx1 = getelementptr inbounds i8, ptr addrspace(200) %out, i64 6
  store i8 %0, ptr addrspace(200) %arrayidx1, align 1, !tbaa !3
  ret void
}

attributes #0 = { norecurse nounwind }

!llvm.module.flags = !{!0, !1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!3 = !{!4, !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
