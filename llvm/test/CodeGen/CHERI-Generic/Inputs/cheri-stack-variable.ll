;; Check the cap-typed @llvm.stacksave.p200 / @llvm.stackrestore.p200
;; intrinsics around a VLA inside a loop, ensuring the saved cap stack
;; pointer is restored each iteration so the stack does not grow without
;; bound.
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -O1 %s -o - | FileCheck %s --check-prefix=CHECK

define void @dynamic_alloca(i32 signext %x) local_unnamed_addr nounwind {
entry:
  %cmp4 = icmp sgt i32 %x, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.cond.cleanup

for.body.lr.ph:                                   ; preds = %entry
  %0 = zext i32 %x to i64
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body, %entry
  ret void

for.body:                                         ; preds = %for.body, %for.body.lr.ph
  %i.05 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %1 = call ptr addrspace(200) @llvm.stacksave.p200()
  %vla = alloca i32, i64 %0, align 4, addrspace(200)
  call void @use_arg(ptr addrspace(200) nonnull %vla)
  call void @llvm.stackrestore.p200(ptr addrspace(200) %1)
  %inc = add nuw nsw i32 %i.05, 1
  %exitcond = icmp eq i32 %inc, %x
  br i1 %exitcond, label %for.cond.cleanup, label %for.body
}

; Function Attrs: nounwind
declare void @use_arg(ptr addrspace(200)) nounwind

declare ptr addrspace(200) @llvm.stacksave.p200()

declare void @llvm.stackrestore.p200(ptr addrspace(200))
