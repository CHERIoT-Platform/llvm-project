; RUN: llc @HYBRID_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=HYBRID
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=PURECAP

define void @storeToPtr1(ptr addrspace(200) nocapture %a, i8 signext %v) nounwind {
entry:
  store i8 %v, ptr addrspace(200) %a, align 1
  ret void
}

define void @storeToPtr2(ptr addrspace(200) nocapture %a, i16 signext %v) nounwind {
entry:
  store i16 %v, ptr addrspace(200) %a, align 2
  ret void
}

define void @storeToPtr4(ptr addrspace(200) nocapture %a, i32 %v) nounwind {
entry:
  store i32 %v, ptr addrspace(200) %a, align 4
  ret void
}

define void @storeToPtr8(ptr addrspace(200) nocapture %a, i64 %v) nounwind {
entry:
  store i64 %v, ptr addrspace(200) %a, align 8
  ret void
}
