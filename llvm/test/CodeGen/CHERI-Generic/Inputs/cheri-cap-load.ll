; RUN: llc @HYBRID_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=HYBRID
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=PURECAP

define zeroext i8 @loadFromPtr1U(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i8, ptr addrspace(200) %a, align 1
  %conv = zext i8 %0 to i32
  %add = add nsw i32 %conv, 12
  %conv1 = trunc i32 %add to i8
  ret i8 %conv1
}

define signext i8 @loadFromPtr1(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i8, ptr addrspace(200) %a, align 1
  %conv2 = zext i8 %0 to i32
  %add = add nsw i32 %conv2, 12
  %conv1 = trunc i32 %add to i8
  ret i8 %conv1
}

define zeroext i16 @loadFromPtr2U(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i16, ptr addrspace(200) %a, align 2
  %conv = zext i16 %0 to i32
  %add = add nsw i32 %conv, 12
  %conv1 = trunc i32 %add to i16
  ret i16 %conv1
}

define signext i16 @loadFromPtr2(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i16, ptr addrspace(200) %a, align 2
  %conv2 = zext i16 %0 to i32
  %add = add nsw i32 %conv2, 12
  %conv1 = trunc i32 %add to i16
  ret i16 %conv1
}

define i32 @loadFromPtr4U(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i32, ptr addrspace(200) %a, align 4
  %add = add i32 %0, 12
  ret i32 %add
}

define i32 @loadFromPtr4(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i32, ptr addrspace(200) %a, align 4
  %add = add nsw i32 %0, 12
  ret i32 %add
}

define i64 @loadFromPtr8U(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i64, ptr addrspace(200) %a, align 8
  %add = add i64 %0, 12
  ret i64 %add
}

define i64 @loadFromPtr8(ptr addrspace(200) nocapture %a) nounwind {
entry:
  %0 = load i64, ptr addrspace(200) %a, align 8
  %add = add nsw i64 %0, 12
  ret i64 %add
}
