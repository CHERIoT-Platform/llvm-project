; RUN: %riscv64_cheri_purecap_opt -passes=cheri-bound-allocas %s -o - -S
; This crash was found compiling postgres (due to a missing depth limitation in CheriPurecapABI.cpp)

target datalayout = "E-m:e-pf200:128:128:128:64-i8:8:32-i16:16:32-i64:64-n32:64-S128-A200-P200-G200"
target triple = "cheri-unknown-freebsd12-purecap"

@a = common local_unnamed_addr addrspace(200) global i32 0, align 4

define signext i32 @b() local_unnamed_addr addrspace(200) nounwind {
entry:
  %d = alloca i8, align 1, addrspace(200)
  %call = tail call signext i32 @c()
  call addrspace(200) void @llvm.lifetime.start.p200(ptr addrspace(200) nonnull %d)
  br label %while.cond

while.cond:                                       ; preds = %if.end, %entry
  %ctrlp.0 = phi ptr addrspace(200) [ %d, %entry ], [ %ctrlp.1, %if.end ]
  %ctrl.0 = phi i8 [ 0, %entry ], [ %ctrl.1, %if.end ]
  %cmp = icmp eq i8 %ctrl.0, 0
  %0 = load i32, ptr addrspace(200) @a, align 4
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %while.cond
  %conv2 = trunc i32 %0 to i8
  store i8 %conv2, ptr addrspace(200) %ctrlp.0, align 1
  br label %if.end

if.end:                                           ; preds = %if.then, %while.cond
  %ctrlp.1 = phi ptr addrspace(200) [ @b, %if.then ], [ %ctrlp.0, %while.cond ]
  %ctrl.1 = phi i8 [ 1, %if.then ], [ %ctrl.0, %while.cond ]
  %or = or i32 %0, 1
  store i32 %or, ptr addrspace(200) @a, align 4
  br label %while.cond
}

declare signext i32 @c(...) local_unnamed_addr addrspace(200)

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p200(ptr addrspace(200) nocapture) addrspace(200) #0

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
