; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -o - -O2 -verify-machineinstrs | FileCheck %s
; Regression test for a crash in Webkit where the tag bit for hash map keys was cleared due to
; inlined memcpy performing an unaligned csd that cleared the tag bit of the key member


declare void @llvm.lifetime.start.p200(i64 immarg, ptr addrspace(200) nocapture) addrspace(200)

declare void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i64, i1 immarg) addrspace(200)

declare void @llvm.lifetime.end.p200(i64 immarg, ptr addrspace(200) nocapture) addrspace(200)

declare ptr addrspace(200) @find_in_table(i32 signext) local_unnamed_addr addrspace(200) nounwind

define i1 @insert_padded_test(ptr addrspace(200) nocapture readonly %key, i32 signext %index) local_unnamed_addr addrspace(200) nounwind {
entry:
  %call = tail call ptr addrspace(200) @find_in_table(i32 signext %index) nounwind
  %0 = bitcast ptr addrspace(200) %call to ptr addrspace(200)
  %1 = bitcast ptr addrspace(200) %key to ptr addrspace(200)
  tail call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %0, ptr addrspace(200) nonnull align 16 %1, i64 32, i1 false)
  ret i1 true
}

; Ensure that we don't attempt to perfom a CIncOffset by 14 followed by an 8byte store for the last few bytes
; This was preivoulsy clearing the tag bit. Perform a 4 byte store followed by a 2 byte one instead!
define i1 @insert_padded_but_copy_only_relevant_bytes(ptr addrspace(200) nocapture readonly %key, i32 signext %index) local_unnamed_addr addrspace(200) nounwind {
entry:
  %call = tail call ptr addrspace(200) @find_in_table(i32 signext %index) nounwind
  %0 = bitcast ptr addrspace(200) %call to ptr addrspace(200)
  %1 = bitcast ptr addrspace(200) %key to ptr addrspace(200)
  tail call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %0, ptr addrspace(200) nonnull align 16 %1, i64 32, i1 false)
  ret i1 true
}

declare ptr addrspace(200) @find_in_table_unpadded(i32 signext) local_unnamed_addr addrspace(200) nounwind

define i1 @insert_no_padding(ptr addrspace(200) nocapture readonly %key, i32 signext %index) local_unnamed_addr addrspace(200) nounwind {
entry:
  %call = tail call ptr addrspace(200) @find_in_table_unpadded(i32 signext %index) nounwind
  %0 = bitcast ptr addrspace(200) %call to ptr addrspace(200)
  %1 = bitcast ptr addrspace(200) %key to ptr addrspace(200)
  tail call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %0, ptr addrspace(200) nonnull align 16 %1, i64 22, i1 false)
  ret i1 true
}
