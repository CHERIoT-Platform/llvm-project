; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; CHECK-NOT: cincoffset   {{.}}, {{.*}}, -1
define i32 @test_hash() addrspace(200) {
  %1 = call addrspace(200) ptr addrspace(200) @llvm.cheri.bounded.stack.cap.i32(ptr addrspace(200) null, i32 1)
  br label %2

2:                                                ; preds = %2, %0
  %3 = phi ptr addrspace(200) [ %5, %2 ], [ %1, %0 ]
  %4 = phi i64 [ %8, %2 ], [ 0, %0 ]
  %5 = getelementptr i8, ptr addrspace(200) %3, i32 1
  %6 = load i8, ptr addrspace(200) %3, align 1
  %7 = zext i8 %6 to i64
  %8 = or i64 1, %7
  %9 = icmp eq ptr addrspace(200) %3, null
  br i1 %9, label %10, label %2

10:                                               ; preds = %2
  store i64 %4, ptr addrspace(200) null, align 8
  ret i32 0
}

; Function Attrs: nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.bounded.stack.cap.i32(ptr addrspace(200), i32) addrspace(200) #0

attributes #0 = { nounwind willreturn memory(none) }

!llvm.module.flags = !{!0}

!0 = !{i32 1, !"target-abi", !"cheriot"}
