; RUN: llc @HYBRID_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=HYBRID
%struct.arg_1000_long = type { [1000 x i64] }
%struct.arg_1000___intcap_t = type { [1000 x ptr addrspace(200)] }

@global_1000_long_struct = common global %struct.arg_1000_long zeroinitializer, align 8
@global_1000___intcap_t_struct = common global %struct.arg_1000___intcap_t zeroinitializer, align 16

; Function Attrs: nounwind
define void @call_1000_long_byval() local_unnamed_addr #0 {
entry:
  tail call void @take_1000_long_byval(ptr nonnull byval(%struct.arg_1000_long) align 8 @global_1000_long_struct) #0
  ret void
}

declare void @other_func(ptr) local_unnamed_addr #0

declare void @take_1000_long_byval(ptr byval(%struct.arg_1000_long) align 8) local_unnamed_addr #0

; Function Attrs: nounwind
define void @call_1000___intcap_t_byval() local_unnamed_addr #0 {
entry:
  tail call void @take_1000___intcap_t_byval(ptr nonnull byval(%struct.arg_1000___intcap_t) align 16 @global_1000___intcap_t_struct) #0
  ret void
}

declare void @take_1000___intcap_t_byval(ptr byval(%struct.arg_1000___intcap_t) align 16) local_unnamed_addr #0

attributes #0 = { nounwind }
