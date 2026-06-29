; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap -verify-machineinstrs -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"


; CHECK: _Z25test_crash_recovery_inneri:
; CHECK: cgetbase
; CHECK: csetaddr
; CHECK-NOT: cincoffset
; CHECK: .LBB4_1:

%struct.____default_malloc_capability_type = type { i32, i32, %struct.AllocatorCapabilityState }
%struct.AllocatorCapabilityState = type { i32, i32, [2 x ptr addrspace(200)] }
%"struct.(anonymous namespace)::DebugContext" = type { [40 x i8] }
%struct.DebugFormatArgument = type { ptr addrspace(200), ptr addrspace(200) }

$_Z9debug_logIJPvEEvPKcDpT_ = comdat any

$_Z9debug_logIJjEEvPKcDpT_ = comdat any

$_Z9debug_logIJEEvPKcDpT_ = comdat any

$_Z9debug_logIJPiEEvPKcDpT_ = comdat any

$__default_malloc_capability = comdat any

@__export.sealing_type.allocator.MallocKey = external dso_local addrspace(200) global i32, align 4
@__default_malloc_capability = linkonce_odr dso_local addrspace(200) global %struct.____default_malloc_capability_type { i32 ptrtoint (ptr addrspace(200) @__export.sealing_type.allocator.MallocKey to i32), i32 0, %struct.AllocatorCapabilityState { i32 4096, i32 0, [2 x ptr addrspace(200)] zeroinitializer } }, section ".sealed_objects", comdat, align 8
@shouldDoubleFault = dso_local addrspace(200) global i8 0, align 1
@shouldSkipFaultingInstruction = dso_local addrspace(200) global i8 0, align 1
@shouldCorruptCSP = dso_local addrspace(200) global i8 0, align 1
@recoveryBehaviour = dso_local addrspace(200) global i32 0, align 4
@.str = private unnamed_addr addrspace(200) constant [33 x i8] c"Detected error in instruction {}\00", align 1
@.str.1 = private unnamed_addr addrspace(200) constant [16 x i8] c"Error cause: {}\00", align 1
@.str.2 = private unnamed_addr addrspace(200) constant [25 x i8] c"Faulting instruction: {}\00", align 1
@.str.3 = private unnamed_addr addrspace(200) constant [24 x i8] c"Triggering double fault\00", align 1
@.str.4 = private unnamed_addr addrspace(200) constant [24 x i8] c"crash_recovery_inner.cc\00", align 1
@.str.5 = private unnamed_addr addrspace(200) constant [26 x i8] c"test_crash_recovery_inner\00", align 1
@.str.6 = private unnamed_addr addrspace(200) constant [51 x i8] c"Trying to store out of bounds in {}, simple unwind\00", align 1
@.str.7 = private unnamed_addr addrspace(200) constant [22 x i8] c"Should be unreachable\00", align 1
@.str.8 = private unnamed_addr addrspace(200) constant [23 x i8] c"Store silently ignored\00", align 1
@.str.9 = private unnamed_addr addrspace(200) constant [54 x i8] c"Trying to fault and double fault in the error handler\00", align 1
@.str.10 = private unnamed_addr addrspace(200) constant [21 x i8] c"Double fault resumed\00", align 1
@.str.11 = private unnamed_addr addrspace(200) constant [53 x i8] c"Trying to fault and corrupt CSP in the error handler\00", align 1
@.str.12 = private unnamed_addr addrspace(200) constant [26 x i8] c"Resumed with exploded CSP\00", align 1
@.str.13 = private unnamed_addr addrspace(200) constant [24 x i8] c"Byte at {} is {}, not 0\00", align 1
@.str.14 = private unnamed_addr addrspace(200) constant [10 x i8] c"Invariant\00", align 1
@_ZTAXtlN12_GLOBAL__N_112DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEE = internal addrspace(200) constant %"struct.(anonymous namespace)::DebugContext" { [40 x i8] c"Crash recovery (inner compartment) test\00" }
@llvm.compiler.used = appending addrspace(200) global [1 x ptr addrspace(200)] [ptr addrspace(200) @__default_malloc_capability], section "llvm.metadata"

; Function Attrs: minsize mustprogress nounwind optsize
define dso_local i32 @compartment_error_handler(ptr addrspace(200) noundef captures(none) %frame, i32 noundef %mcause, i32 noundef %mtval) local_unnamed_addr addrspace(200) #0 section ".compartment_error_handler" {
entry:
  %0 = load ptr addrspace(200), ptr addrspace(200) %frame, align 8, !tbaa !6
  tail call void @_Z9debug_logIJPvEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str, ptr addrspace(200) noundef %0) #8
  tail call void @_Z9debug_logIJjEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str.1, i32 noundef %mcause) #8
  %1 = load volatile i8, ptr addrspace(200) @shouldSkipFaultingInstruction, align 1, !tbaa !11, !range !13, !noundef !14
  %loadedv = trunc nuw i8 %1 to i1
  br i1 %loadedv, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %2 = tail call ptr addrspace(200) @llvm.cheri.pcc.get()
  %3 = load ptr addrspace(200), ptr addrspace(200) %frame, align 8, !tbaa !6
  %4 = tail call noundef i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200) %3)
  %5 = tail call ptr addrspace(200) @llvm.cheri.cap.address.set.i32(ptr addrspace(200) %2, i32 %4)
  %6 = load i32, ptr addrspace(200) %5, align 1
  tail call void @_Z9debug_logIJjEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str.2, i32 noundef %6) #8
  %and = and i32 %6, 3
  %cmp = icmp eq i32 %and, 3
  %cond = select i1 %cmp, i32 4, i32 2
  %7 = load ptr addrspace(200), ptr addrspace(200) %frame, align 8, !tbaa !6
  %add.ptr = getelementptr inbounds nuw i8, ptr addrspace(200) %7, i32 %cond
  store ptr addrspace(200) %add.ptr, ptr addrspace(200) %frame, align 8, !tbaa !6
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %8 = load volatile i8, ptr addrspace(200) @shouldDoubleFault, align 1, !tbaa !11, !range !13, !noundef !14
  %loadedv10 = trunc nuw i8 %8 to i1
  br i1 %loadedv10, label %if.then11, label %if.end12

if.then11:                                        ; preds = %if.end
  tail call void @_Z9debug_logIJEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str.3) #8
  %9 = tail call ptr addrspace(200) @llvm.cheri.pcc.get()
  store i8 1, ptr addrspace(200) %9, align 1, !tbaa !15
  br label %if.end12

if.end12:                                         ; preds = %if.then11, %if.end
  %10 = load volatile i8, ptr addrspace(200) @shouldCorruptCSP, align 1, !tbaa !11, !range !13, !noundef !14
  %loadedv13 = trunc nuw i8 %10 to i1
  br i1 %loadedv13, label %if.then14, label %if.end15

if.then14:                                        ; preds = %if.end12
  %11 = tail call ptr addrspace(200) @llvm.returnaddress.p200(i32 0)
  tail call void asm sideeffect "cmove csp, cnull\0Acjr $0\0A", "C"(ptr addrspace(200) %11) #9, !srcloc !16
  br label %if.end15

if.end15:                                         ; preds = %if.then14, %if.end12
  %12 = load volatile i32, ptr addrspace(200) @recoveryBehaviour, align 4, !tbaa !17
  ret i32 %12
}

; Function Attrs: minsize mustprogress nounwind optsize
define linkonce_odr dso_local void @_Z9debug_logIJPvEEvPKcDpT_(ptr addrspace(200) noundef %fmt, ptr addrspace(200) noundef %args) local_unnamed_addr addrspace(200) #0 comdat {
entry:
  %arguments.i = alloca [1 x %struct.DebugFormatArgument], align 8, addrspace(200)
  tail call void asm sideeffect "", "~{memory}"() #9, !srcloc !19
  call void @llvm.lifetime.start.p200(i64 16, ptr addrspace(200) nonnull %arguments.i) #9
  store ptr addrspace(200) %args, ptr addrspace(200) %arguments.i, align 8, !tbaa !20
  %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i.i = getelementptr inbounds nuw i8, ptr addrspace(200) %arguments.i, i32 8
  store ptr addrspace(200) getelementptr (i8, ptr addrspace(200) null, i32 8), ptr addrspace(200) %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i.i, align 8, !tbaa !20
  call cheriot_librarycallcc void @_Z23debug_log_message_writePKcS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef nonnull @_ZTAXtlN12_GLOBAL__N_112DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEE, ptr addrspace(200) noundef %fmt, ptr addrspace(200) noundef nonnull %arguments.i, i32 noundef 1) #10
  call void @llvm.lifetime.end.p200(i64 16, ptr addrspace(200) nonnull %arguments.i) #9
  call void asm sideeffect "", "~{memory}"() #9, !srcloc !22
  ret void
}

; Function Attrs: minsize mustprogress nounwind optsize
define linkonce_odr dso_local void @_Z9debug_logIJjEEvPKcDpT_(ptr addrspace(200) noundef %fmt, i32 noundef %args) local_unnamed_addr addrspace(200) #0 comdat {
entry:
  %arguments.i = alloca [1 x %struct.DebugFormatArgument], align 8, addrspace(200)
  tail call void asm sideeffect "", "~{memory}"() #9, !srcloc !19
  call void @llvm.lifetime.start.p200(i64 16, ptr addrspace(200) nonnull %arguments.i) #9
  %0 = getelementptr i8, ptr addrspace(200) null, i32 %args
  store ptr addrspace(200) %0, ptr addrspace(200) %arguments.i, align 8, !tbaa !20
  %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i.i = getelementptr inbounds nuw i8, ptr addrspace(200) %arguments.i, i32 8
  store ptr addrspace(200) getelementptr (i8, ptr addrspace(200) null, i32 3), ptr addrspace(200) %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i.i, align 8, !tbaa !20
  call cheriot_librarycallcc void @_Z23debug_log_message_writePKcS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef nonnull @_ZTAXtlN12_GLOBAL__N_112DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEE, ptr addrspace(200) noundef %fmt, ptr addrspace(200) noundef nonnull %arguments.i, i32 noundef 1) #10
  call void @llvm.lifetime.end.p200(i64 16, ptr addrspace(200) nonnull %arguments.i) #9
  call void asm sideeffect "", "~{memory}"() #9, !srcloc !22
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p200(i64 immarg, ptr addrspace(200) captures(none)) addrspace(200) #1

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.pcc.get() addrspace(200) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p200(i64 immarg, ptr addrspace(200) captures(none)) addrspace(200) #1

; Function Attrs: minsize mustprogress nounwind optsize
define linkonce_odr dso_local void @_Z9debug_logIJEEvPKcDpT_(ptr addrspace(200) noundef %fmt) local_unnamed_addr addrspace(200) #0 comdat {
entry:
  tail call void asm sideeffect "", "~{memory}"() #9, !srcloc !19
  tail call cheriot_librarycallcc void @_Z23debug_log_message_writePKcS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef nonnull @_ZTAXtlN12_GLOBAL__N_112DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEE, ptr addrspace(200) noundef %fmt, ptr addrspace(200) noundef null, i32 noundef 0) #10
  tail call void asm sideeffect "", "~{memory}"() #9, !srcloc !22
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.returnaddress.p200(i32 immarg) addrspace(200) #3

; Function Attrs: minsize mustprogress nounwind optsize
define dso_local cheriot_compartmentcalleecc noundef ptr addrspace(200) @_Z25test_crash_recovery_inneri(i32 noundef %option) local_unnamed_addr addrspace(200) #4 {
entry:
  %x = alloca [16 x i32], align 4, addrspace(200)
  %0 = tail call ptr addrspace(200) asm "", "={c2}"() #11, !srcloc !23
  %1 = tail call noundef i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200) %0)
  %2 = tail call noundef i32 @llvm.cheri.cap.base.get.i32(ptr addrspace(200) %0)
  %3 = tail call ptr addrspace(200) @llvm.cheri.cap.address.set.i32(ptr addrspace(200) %0, i32 %2)
  %4 = xor i32 %2, -1
  %sub7.i = add i32 %1, %4
  br label %for.cond.i

for.cond.i:                                       ; preds = %for.body.i, %entry
  %i.0.i = phi i32 [ %sub7.i, %entry ], [ %dec.i, %for.body.i ]
  %cmp.i = icmp sgt i32 %i.0.i, 0
  br i1 %cmp.i, label %for.body.i, label %_Z11check_stackN12_GLOBAL__N_114SourceLocationE.exit

for.body.i:                                       ; preds = %for.cond.i
  %arrayidx.i.i = getelementptr inbounds nuw i8, ptr addrspace(200) %3, i32 %i.0.i
  %5 = load i8, ptr addrspace(200) %arrayidx.i.i, align 1, !tbaa !15
  %cmp9.not.i = icmp eq i8 %5, 0
  %dec.i = add nsw i32 %i.0.i, -1
  br i1 %cmp9.not.i, label %for.cond.i, label %if.then.i.i, !llvm.loop !24

if.then.i.i:                                      ; preds = %for.body.i
  tail call fastcc void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJPchEEEvPKcS9_S9_iS9_DpT_(ptr addrspace(200) noundef nonnull @.str.4, ptr addrspace(200) noundef nonnull @.str.5, i32 noundef 56, ptr addrspace(200) noundef nonnull %arrayidx.i.i, i8 noundef zeroext %5) #8
  tail call void @llvm.trap()
  unreachable

_Z11check_stackN12_GLOBAL__N_114SourceLocationE.exit: ; preds = %for.cond.i
  call void @llvm.lifetime.start.p200(i64 64, ptr addrspace(200) nonnull %x) #9
  switch i32 %option, label %cleanup [
    i32 0, label %sw.bb
    i32 1, label %sw.bb2
    i32 2, label %sw.bb3
    i32 3, label %sw.bb6
  ]

sw.bb:                                            ; preds = %_Z11check_stackN12_GLOBAL__N_114SourceLocationE.exit
  store volatile i8 0, ptr addrspace(200) @shouldDoubleFault, align 1, !tbaa !11
  store volatile i8 0, ptr addrspace(200) @shouldSkipFaultingInstruction, align 1, !tbaa !11
  store volatile i8 0, ptr addrspace(200) @shouldCorruptCSP, align 1, !tbaa !11
  call void @_Z9debug_logIJPiEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str.6, ptr addrspace(200) noundef nonnull %x) #8
  store volatile i32 1, ptr addrspace(200) @recoveryBehaviour, align 4, !tbaa !17
  fence syncscope("singlethread") seq_cst
  %arrayidx.i = getelementptr inbounds nuw i8, ptr addrspace(200) %x, i32 64
  store i32 0, ptr addrspace(200) %arrayidx.i, align 4, !tbaa !26
  fence syncscope("singlethread") seq_cst
  call fastcc void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJEEEvPKcS8_S8_iS8_DpT_(ptr addrspace(200) noundef nonnull @.str.4, ptr addrspace(200) noundef nonnull @.str.5, i32 noundef 75, ptr addrspace(200) noundef nonnull @.str.7) #8
  call void @llvm.trap()
  unreachable

sw.bb2:                                           ; preds = %_Z11check_stackN12_GLOBAL__N_114SourceLocationE.exit
  store volatile i8 0, ptr addrspace(200) @shouldDoubleFault, align 1, !tbaa !11
  store volatile i8 1, ptr addrspace(200) @shouldSkipFaultingInstruction, align 1, !tbaa !11
  store volatile i8 0, ptr addrspace(200) @shouldCorruptCSP, align 1, !tbaa !11
  store volatile i32 0, ptr addrspace(200) @recoveryBehaviour, align 4, !tbaa !17
  fence syncscope("singlethread") seq_cst
  %arrayidx.i27 = getelementptr inbounds nuw i8, ptr addrspace(200) %x, i32 64
  store i32 0, ptr addrspace(200) %arrayidx.i27, align 4, !tbaa !26
  fence syncscope("singlethread") seq_cst
  tail call void @_Z9debug_logIJEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str.8) #8
  br label %cleanup

sw.bb3:                                           ; preds = %_Z11check_stackN12_GLOBAL__N_114SourceLocationE.exit
  store volatile i8 1, ptr addrspace(200) @shouldDoubleFault, align 1, !tbaa !11
  store volatile i8 1, ptr addrspace(200) @shouldSkipFaultingInstruction, align 1, !tbaa !11
  store volatile i8 0, ptr addrspace(200) @shouldCorruptCSP, align 1, !tbaa !11
  store volatile i32 0, ptr addrspace(200) @recoveryBehaviour, align 4, !tbaa !17
  tail call void @_Z9debug_logIJEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str.9) #8
  fence syncscope("singlethread") seq_cst
  %arrayidx.i28 = getelementptr inbounds nuw i8, ptr addrspace(200) %x, i32 64
  store i32 0, ptr addrspace(200) %arrayidx.i28, align 4, !tbaa !26
  fence syncscope("singlethread") seq_cst
  tail call fastcc void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJEEEvPKcS8_S8_iS8_DpT_(ptr addrspace(200) noundef nonnull @.str.4, ptr addrspace(200) noundef nonnull @.str.5, i32 noundef 93, ptr addrspace(200) noundef nonnull @.str.10) #8
  tail call void @llvm.trap()
  unreachable

sw.bb6:                                           ; preds = %_Z11check_stackN12_GLOBAL__N_114SourceLocationE.exit
  store volatile i8 0, ptr addrspace(200) @shouldDoubleFault, align 1, !tbaa !11
  store volatile i8 1, ptr addrspace(200) @shouldSkipFaultingInstruction, align 1, !tbaa !11
  store volatile i8 1, ptr addrspace(200) @shouldCorruptCSP, align 1, !tbaa !11
  store volatile i32 0, ptr addrspace(200) @recoveryBehaviour, align 4, !tbaa !17
  tail call void @_Z9debug_logIJEEvPKcDpT_(ptr addrspace(200) noundef nonnull @.str.11) #8
  fence syncscope("singlethread") seq_cst
  %arrayidx.i32 = getelementptr inbounds nuw i8, ptr addrspace(200) %x, i32 64
  store i32 0, ptr addrspace(200) %arrayidx.i32, align 4, !tbaa !26
  fence syncscope("singlethread") seq_cst
  tail call fastcc void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJEEEvPKcS8_S8_iS8_DpT_(ptr addrspace(200) noundef nonnull @.str.4, ptr addrspace(200) noundef nonnull @.str.5, i32 noundef 102, ptr addrspace(200) noundef nonnull @.str.12) #8
  tail call void @llvm.trap()
  unreachable

cleanup:                                          ; preds = %_Z11check_stackN12_GLOBAL__N_114SourceLocationE.exit, %sw.bb2
  call void @llvm.lifetime.end.p200(i64 64, ptr addrspace(200) nonnull %x) #9
  ret ptr addrspace(200) null
}

; Function Attrs: minsize mustprogress nounwind optsize
define linkonce_odr dso_local void @_Z9debug_logIJPiEEvPKcDpT_(ptr addrspace(200) noundef %fmt, ptr addrspace(200) noundef %args) local_unnamed_addr addrspace(200) #0 comdat {
entry:
  %arguments.i = alloca [1 x %struct.DebugFormatArgument], align 8, addrspace(200)
  tail call void asm sideeffect "", "~{memory}"() #9, !srcloc !19
  call void @llvm.lifetime.start.p200(i64 16, ptr addrspace(200) nonnull %arguments.i) #9
  store ptr addrspace(200) %args, ptr addrspace(200) %arguments.i, align 8, !tbaa !20
  %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i.i = getelementptr inbounds nuw i8, ptr addrspace(200) %arguments.i, i32 8
  store ptr addrspace(200) getelementptr (i8, ptr addrspace(200) null, i32 8), ptr addrspace(200) %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i.i, align 8, !tbaa !20
  call cheriot_librarycallcc void @_Z23debug_log_message_writePKcS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef nonnull @_ZTAXtlN12_GLOBAL__N_112DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEE, ptr addrspace(200) noundef %fmt, ptr addrspace(200) noundef nonnull %arguments.i, i32 noundef 1) #10
  call void @llvm.lifetime.end.p200(i64 16, ptr addrspace(200) nonnull %arguments.i) #9
  call void asm sideeffect "", "~{memory}"() #9, !srcloc !22
  ret void
}

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.cheri.cap.address.get.i32(ptr addrspace(200)) addrspace(200) #2

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.cheri.cap.base.get.i32(ptr addrspace(200)) addrspace(200) #2

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.cap.address.set.i32(ptr addrspace(200), i32) addrspace(200) #2

; Function Attrs: inlinehint minsize mustprogress nounwind optsize
define internal fastcc void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJPchEEEvPKcS9_S9_iS9_DpT_(ptr addrspace(200) noundef %file, ptr addrspace(200) noundef %function, i32 noundef %line, ptr addrspace(200) noundef nonnull %args, i8 noundef zeroext %args1) unnamed_addr addrspace(200) #5 align 2 {
entry:
  %arguments = alloca [2 x %struct.DebugFormatArgument], align 8, addrspace(200)
  call void @llvm.lifetime.start.p200(i64 32, ptr addrspace(200) nonnull %arguments) #9
  %conv.i = zext i8 %args1 to i32
  %0 = getelementptr i8, ptr addrspace(200) null, i32 %conv.i
  %arrayidx.i = getelementptr inbounds nuw i8, ptr addrspace(200) %arguments, i32 16
  store ptr addrspace(200) %0, ptr addrspace(200) %arrayidx.i, align 8, !tbaa !20
  %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i = getelementptr inbounds nuw i8, ptr addrspace(200) %arguments, i32 24
  store ptr addrspace(200) getelementptr (i8, ptr addrspace(200) null, i32 3), ptr addrspace(200) %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i, align 8, !tbaa !20
  store ptr addrspace(200) %args, ptr addrspace(200) %arguments, align 8, !tbaa !20
  %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i6 = getelementptr inbounds nuw i8, ptr addrspace(200) %arguments, i32 8
  store ptr addrspace(200) getelementptr (i8, ptr addrspace(200) null, i32 8), ptr addrspace(200) %ref.tmp.sroa.4.0.arrayidx.sroa_idx.i6, align 8, !tbaa !20
  call cheriot_librarycallcc void @_Z20debug_report_failurePKcS0_S0_iS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef nonnull @.str.14, ptr addrspace(200) noundef %file, ptr addrspace(200) noundef %function, i32 noundef %line, ptr addrspace(200) noundef nonnull @.str.13, ptr addrspace(200) noundef nonnull %arguments, i32 noundef 2) #10
  call void @llvm.lifetime.end.p200(i64 32, ptr addrspace(200) nonnull %arguments) #9
  ret void
}

; Function Attrs: cold noreturn nounwind memory(inaccessiblemem: write)
declare void @llvm.trap() addrspace(200) #6

; Function Attrs: minsize optsize
declare dso_local cheriot_librarycallcc void @_Z20debug_report_failurePKcS0_S0_iS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef, ptr addrspace(200) noundef, ptr addrspace(200) noundef, i32 noundef, ptr addrspace(200) noundef, ptr addrspace(200) noundef, i32 noundef) local_unnamed_addr addrspace(200) #7

; Function Attrs: minsize optsize
declare dso_local cheriot_librarycallcc void @_Z23debug_log_message_writePKcS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef, ptr addrspace(200) noundef, ptr addrspace(200) noundef, i32 noundef) local_unnamed_addr addrspace(200) #7

; Function Attrs: inlinehint minsize mustprogress nounwind optsize
define internal fastcc void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj40EEEtlA40_cLc67ELc114ELc97ELc115ELc104ELc32ELc114ELc101ELc99ELc111ELc118ELc101ELc114ELc121ELc32ELc40ELc105ELc110ELc110ELc101ELc114ELc32ELc99ELc111ELc109ELc112ELc97ELc114ELc116ELc109ELc101ELc110ELc116ELc41ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJEEEvPKcS8_S8_iS8_DpT_(ptr addrspace(200) noundef %file, ptr addrspace(200) noundef %function, i32 noundef %line, ptr addrspace(200) noundef %fmt) unnamed_addr addrspace(200) #5 align 2 {
entry:
  %arguments = alloca [0 x %struct.DebugFormatArgument], align 8, addrspace(200)
  call void @llvm.lifetime.start.p200(i64 0, ptr addrspace(200) nonnull %arguments) #9
  call cheriot_librarycallcc void @_Z20debug_report_failurePKcS0_S0_iS0_P19DebugFormatArgumentj(ptr addrspace(200) noundef nonnull @.str.14, ptr addrspace(200) noundef %file, ptr addrspace(200) noundef %function, i32 noundef %line, ptr addrspace(200) noundef %fmt, ptr addrspace(200) noundef nonnull %arguments, i32 noundef 0) #10
  call void @llvm.lifetime.end.p200(i64 0, ptr addrspace(200) nonnull %arguments) #9
  ret void
}

attributes #0 = { minsize mustprogress nounwind optsize "cheri-compartment"="crash_recovery_inner" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { mustprogress nofree nosync nounwind willreturn memory(none) }
attributes #3 = { mustprogress nocallback nofree nosync nounwind willreturn memory(none) }
attributes #4 = { minsize mustprogress nounwind optsize "cheri-compartment"="crash_recovery_inner" "interrupt-state"="enabled" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #5 = { inlinehint minsize mustprogress nounwind optsize "cheri-compartment"="crash_recovery_inner" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #6 = { cold noreturn nounwind memory(inaccessiblemem: write) }
attributes #7 = { minsize optsize "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #8 = { minsize optsize "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" }
attributes #9 = { nounwind }
attributes #10 = { minsize nounwind optsize "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" }
attributes #11 = { nounwind memory(none) }

!llvm.module.flags = !{!0, !1, !2, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_zca1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
!5 = !{!"clang version 21.1.8 (git@github.com:resistor/llvm-project-1.git 9f296e215fcb9108405f5c08cd33979a3c8d7a13)"}
!6 = !{!7, !8, i64 0}
!7 = !{!"_ZTS10ErrorState", !8, i64 0, !9, i64 8}
!8 = !{!"any pointer", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C++ TBAA"}
!11 = !{!12, !12, i64 0}
!12 = !{!"bool", !9, i64 0}
!13 = !{i8 0, i8 2}
!14 = !{}
!15 = !{!9, !9, i64 0}
!16 = !{i64 1569, i64 1606}
!17 = !{!18, !18, i64 0}
!18 = !{!"_ZTS22ErrorRecoveryBehaviour", !9, i64 0}
!19 = !{i64 58332}
!20 = !{!21, !21, i64 0}
!21 = !{!"unsigned __intcap", !9, i64 0}
!22 = !{i64 58804}
!23 = !{i64 26348}
!24 = distinct !{!24, !25}
!25 = !{!"llvm.loop.mustprogress"}
!26 = !{!27, !27, i64 0}
!27 = !{!"int", !9, i64 0}
