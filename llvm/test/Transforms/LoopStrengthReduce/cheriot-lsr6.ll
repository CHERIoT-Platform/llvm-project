; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; CHECK-LABEL: %bb14
; CHECK-NOT: cincoffset	{{.*}}, -4

@alloc_9588fd9eb89b71aecf86d2ab3280d014 = external hidden unnamed_addr addrspace(200) constant <{ ptr addrspace(200), [4 x i8], [4 x i8], [8 x i8] }>, align 8
@alloc_45c28facb1bb8016d3ea9e1d6f907c1e = external hidden unnamed_addr addrspace(200) constant <{ ptr addrspace(200), [4 x i8], [4 x i8], [8 x i8] }>, align 8
@alloc_a9c4d6ab0ff0d6fb90dda7bf52fd7aca = external hidden unnamed_addr addrspace(200) constant <{ ptr addrspace(200), [4 x i8], [4 x i8], [8 x i8] }>, align 8
@alloc_6d4cd78cee70c5eec05fa83141d7b53c = external hidden unnamed_addr addrspace(200) constant <{ ptr addrspace(200), [4 x i8], [4 x i8], [8 x i8] }>, align 8

; Function Attrs: minsize noinline nounwind optsize
define hidden fastcc noundef nonnull align 4 ptr addrspace(200) @_RNvMCs2LzHmOg8pe3_8iter_revNtB2_8Big32x408mul_pow2(ptr addrspace(200) noalias noundef nonnull returned align 4 captures(ret: address, provenance) dereferenceable(164) %self, i32 noundef signext %bits) unnamed_addr addrspace(200) #0 {
start:
  %digits11 = lshr i32 %bits, 5
  %bits1 = and i32 %bits, 31
  %0 = getelementptr inbounds nuw i8, ptr addrspace(200) %self, i32 160
  %_6 = load i32, ptr addrspace(200) %0, align 4, !noundef !2
  %1 = add i32 %_6, %digits11
  %_7.not = icmp eq i32 %bits1, 0
  br i1 %_7.not, label %bb16, label %bb1

bb16:                                             ; preds = %bb15, %start
  %sz.sroa.0.0 = phi i32 [ %sz.sroa.0.1, %bb15 ], [ %1, %start ]
  store i32 %sz.sroa.0.0, ptr addrspace(200) %0, align 4
  ret ptr addrspace(200) %self

bb1:                                              ; preds = %start
  %_11 = add i32 %1, -1
  %_12 = icmp ult i32 %_11, 40
  br i1 %_12, label %bb2, label %panic

bb2:                                              ; preds = %bb1
  %2 = getelementptr inbounds nuw i32, ptr addrspace(200) %self, i32 %_11
  %_10 = load i32, ptr addrspace(200) %2, align 4, !noundef !2
  %_13 = sub i32 0, %bits
  %3 = and i32 %_13, 31
  %overflow = lshr i32 %_10, %3
  %_14.not = icmp eq i32 %overflow, 0
  br i1 %_14.not, label %bb5, label %bb3

panic:                                            ; preds = %bb1
  tail call addrspace(200) void @_RNvNtCsHqy3LjcUoC_4core9panicking18panic_bounds_check(i32 noundef signext %_11, i32 noundef signext 40, ptr addrspace(200) noalias noundef readonly align 8 captures(address, read_provenance) dereferenceable(24) @alloc_9588fd9eb89b71aecf86d2ab3280d014) #2
  unreachable

bb5:                                              ; preds = %bb4, %bb2
  %sz.sroa.0.1 = phi i32 [ %5, %bb4 ], [ %1, %bb2 ]
  %_19 = add nuw nsw i32 %digits11, 1
  br label %bb8

bb3:                                              ; preds = %bb2
  %_15 = icmp samesign ult i32 %1, 40
  br i1 %_15, label %bb4, label %panic2

bb4:                                              ; preds = %bb3
  %4 = getelementptr inbounds nuw i32, ptr addrspace(200) %self, i32 %1
  store i32 %overflow, ptr addrspace(200) %4, align 4
  %5 = add nuw nsw i32 %1, 1
  br label %bb5

panic2:                                           ; preds = %bb3
  tail call addrspace(200) void @_RNvNtCsHqy3LjcUoC_4core9panicking18panic_bounds_check(i32 noundef signext %1, i32 noundef signext 40, ptr addrspace(200) noalias noundef readonly align 8 captures(address, read_provenance) dereferenceable(24) @alloc_45c28facb1bb8016d3ea9e1d6f907c1e) #2
  unreachable

bb8:                                              ; preds = %bb14, %bb5
  %_26 = phi i32 [ %_10, %bb5 ], [ %_29, %bb14 ]
  %iter.sroa.4.0 = phi i32 [ %1, %bb5 ], [ %_0.i1.i.i.i, %bb14 ]
  %_0.i.i.i.i = icmp samesign ult i32 %_19, %iter.sroa.4.0
  br i1 %_0.i.i.i.i, label %bb13, label %bb12

bb12:                                             ; preds = %bb8
  %_32 = icmp ult i32 %bits, 1280
  br i1 %_32, label %bb15, label %panic3

bb15:                                             ; preds = %bb12
  %6 = getelementptr inbounds nuw i32, ptr addrspace(200) %self, i32 %digits11
  %7 = load i32, ptr addrspace(200) %6, align 4, !noundef !2
  %8 = shl i32 %7, %bits1
  store i32 %8, ptr addrspace(200) %6, align 4
  br label %bb16

panic3:                                           ; preds = %bb12
  tail call addrspace(200) void @_RNvNtCsHqy3LjcUoC_4core9panicking18panic_bounds_check(i32 noundef signext %digits11, i32 noundef signext 40, ptr addrspace(200) noalias noundef readonly align 8 captures(address, read_provenance) dereferenceable(24) @alloc_a9c4d6ab0ff0d6fb90dda7bf52fd7aca) #2
  unreachable

bb13:                                             ; preds = %bb8
  %_30 = add nsw i32 %iter.sroa.4.0, -2
  %_31 = icmp ult i32 %_30, 40
  br i1 %_31, label %bb14, label %panic5

bb14:                                             ; preds = %bb13
  %_0.i1.i.i.i = add nsw i32 %iter.sroa.4.0, -1
  %9 = getelementptr inbounds nuw i32, ptr addrspace(200) %self, i32 %_0.i1.i.i.i
  %_25 = shl i32 %_26, %bits1
  %10 = getelementptr inbounds nuw i32, ptr addrspace(200) %self, i32 %_30
  %_29 = load i32, ptr addrspace(200) %10, align 4, !noundef !2
  %_28 = lshr i32 %_29, %3
  %11 = or i32 %_28, %_25
  store i32 %11, ptr addrspace(200) %9, align 4
  br label %bb8

panic5:                                           ; preds = %bb13
  tail call addrspace(200) void @_RNvNtCsHqy3LjcUoC_4core9panicking18panic_bounds_check(i32 noundef signext -1, i32 noundef signext 40, ptr addrspace(200) noalias noundef readonly align 8 captures(address, read_provenance) dereferenceable(24) @alloc_6d4cd78cee70c5eec05fa83141d7b53c) #2
  unreachable
}

; Function Attrs: cold minsize noinline noreturn nounwind optsize
declare dso_local void @_RNvNtCsHqy3LjcUoC_4core9panicking18panic_bounds_check(i32 noundef signext, i32 noundef signext, ptr addrspace(200) noalias noundef readonly align 8 captures(address, read_provenance) dereferenceable(24)) unnamed_addr addrspace(200) #1

attributes #0 = { minsize noinline nounwind optsize "target-cpu"="cheriot" "target-features"="+32bit,+c,+zca,+e,+m,+xcheriot" }
attributes #1 = { cold minsize noinline noreturn nounwind optsize "target-cpu"="cheriot" "target-features"="+32bit,+c,+zca,+e,+m,+xcheriot" }
attributes #2 = { noinline noreturn nounwind }

!llvm.ident = !{!0}
!llvm.module.flags = !{!1}

!0 = !{!"rustc version 1.95.0-dev"}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{}
