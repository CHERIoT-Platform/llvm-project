; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot  %s -mattr=+xcheri,+cap-mode -o - | FileCheck %s
; ModuleID = 'sret.cc'
source_filename = "sret.cc"
target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-unknown"

%struct.Smallish = type { i32, i32, i32 }
%struct.Big = type { [128 x i8] }

; Function Attrs: minsize mustprogress nounwind optsize
define hidden i32 @_Z1hv() local_unnamed_addr addrspace(200) #0 {
entry:
  %ref.tmp = alloca %struct.Smallish, align 4, addrspace(200)
  %0 = bitcast %struct.Smallish addrspace(200)* %ref.tmp to i8 addrspace(200)*
  call void @llvm.lifetime.start.p200i8(i64 12, i8 addrspace(200)* nonnull %0) #3
  ; CHECK-LABEL: _Z1hv:
  ; Make sure that a 3-word struct is zeroed without a memset call.
  ; CHECK: csw	zero, 8(ca0)
  ; CHECK: csw	zero, 4(ca0)
  ; CHECK: csw	zero, 0(ca0)
  ; CHECK: auipcc	ct1, %cheriot_compartment_hi(__import_sret__Z1fv)
  notail call chericcallcc void @_Z1fv(%struct.Smallish addrspace(200)* nonnull sret(%struct.Smallish) align 4 %ref.tmp) #4
  %foo.sroa.0.0..sroa_idx = getelementptr inbounds %struct.Smallish, %struct.Smallish addrspace(200)* %ref.tmp, i32 0, i32 0
  %foo.sroa.0.0.copyload = load i32, i32 addrspace(200)* %foo.sroa.0.0..sroa_idx, align 4, !tbaa.struct !4
  call void @llvm.lifetime.end.p200i8(i64 12, i8 addrspace(200)* nonnull %0) #3
  ret i32 %foo.sroa.0.0.copyload
}

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p200i8(i64 immarg, i8 addrspace(200)* nocapture) addrspace(200) #1

; Function Attrs: minsize optsize
declare dso_local chericcallcc void @_Z1fv(%struct.Smallish addrspace(200)* sret(%struct.Smallish) align 4) local_unnamed_addr addrspace(200) #2

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p200i8(i64 immarg, i8 addrspace(200)* nocapture) addrspace(200) #1

; Function Attrs: minsize mustprogress nounwind optsize
define hidden i32 @_Z1iv() local_unnamed_addr addrspace(200) #0 {
entry:
  %ref.tmp = alloca %struct.Big, align 1, addrspace(200)
  %0 = getelementptr inbounds %struct.Big, %struct.Big addrspace(200)* %ref.tmp, i32 0, i32 0, i32 0
  call void @llvm.lifetime.start.p200i8(i64 128, i8 addrspace(200)* nonnull %0) #3
  ; CHECK-LABEL: _Z1iv:
  ; Check that we do a proper memset for a big struct.
  ; CHECK: 	auipcc	ct2, %cheriot_compartment_hi(__library_import_libcalls__Z6memsetPvij)
  notail call chericcallcc void @_Z1gv(%struct.Big addrspace(200)* nonnull sret(%struct.Big) align 1 %ref.tmp) #4
  %foo.sroa.0.0.copyload = load i8, i8 addrspace(200)* %0, align 1, !tbaa.struct !9
  call void @llvm.lifetime.end.p200i8(i64 128, i8 addrspace(200)* nonnull %0) #3
  %conv = zext i8 %foo.sroa.0.0.copyload to i32
  ret i32 %conv
}

; Function Attrs: minsize optsize
declare dso_local chericcallcc void @_Z1gv(%struct.Big addrspace(200)* sret(%struct.Big) align 1) local_unnamed_addr addrspace(200) #2

attributes #0 = { minsize mustprogress nounwind optsize "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+relax,+xcheri,+xcheri-rvc,-64bit,-save-restore" }
attributes #1 = { argmemonly mustprogress nofree nosync nounwind willreturn }
attributes #2 = { minsize optsize "cheri-compartment"="sret" "frame-pointer"="none" "interrupt-state"="enabled" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+relax,+xcheri,+xcheri-rvc,-64bit,-save-restore" }
attributes #3 = { nounwind }
attributes #4 = { minsize nobuiltin nounwind optsize "no-builtins" }

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

