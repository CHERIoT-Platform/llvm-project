;; This test assumes cheri128, since on 256 a 16 byte aligned pointer won't be expanded
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -O1 %s -o - | FileCheck %s --check-prefix=PURECAP

define void @zero64(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 64, i1 false)
  ret void
}

define void @zero64_unaligned(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 8 %out, i8 0, i64 64, i1 false)
  ret void
}

define void @zero65(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 65, i1 false)
  ret void
}

define void @zero66(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 66, i1 false)
  ret void
}

define void @zero67(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 67, i1 false)
  ret void
}

define void @zero68(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 68, i1 false)
  ret void
}

define void @zero69(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 69, i1 false)
  ret void
}

define void @zero70(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 70, i1 false)
  ret void
}

define void @zero71(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 71, i1 false)
  ret void
}

define void @zero72(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 72, i1 false)
  ret void
}

define void @zero73(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 73, i1 false)
  ret void
}

define void @zero74(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 74, i1 false)
  ret void
}

define void @zero75(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 75, i1 false)
  ret void
}

define void @zero76(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 76, i1 false)
  ret void
}

define void @zero77(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 77, i1 false)
  ret void
}

define void @zero78(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 78, i1 false)
  ret void
}

define void @zero79(ptr addrspace(200) nocapture %out) local_unnamed_addr nounwind {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 16 %out, i8 0, i64 79, i1 false)
  ret void
}

declare void @llvm.memset.p200.i64(ptr addrspace(200) nocapture writeonly, i8, i64, i1 immarg) #0

attributes #0 = { nocallback nofree nounwind willreturn memory(argmem: write) }
