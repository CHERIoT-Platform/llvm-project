; RUN: opt -passes=instcombine -S < %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

%struct.____default_malloc_capability_type = type { i32, i32, %struct.AllocatorCapabilityState }
%struct.AllocatorCapabilityState = type { i32, i32, [2 x ptr addrspace(200)] }
%struct.Timeout = type { i32, i32 }

; CHECK: @_Z9do_thingsv
; CHECK: entry:
; CHECK-NOT: heap_allocate
; CHECK: for.cond.cleanup:
; CHECK-NOT: heap_free
; CHECK: for.body

@__export.sealing_type.allocator.MallocKey = external dso_local addrspace(200) global i32, align 4
@__default_malloc_capability = dso_local addrspace(200) global %struct.____default_malloc_capability_type { i32 ptrtoint (ptr addrspace(200) @__export.sealing_type.allocator.MallocKey to i32), i32 0, %struct.AllocatorCapabilityState { i32 4096, i32 0, [2 x ptr addrspace(200)] zeroinitializer } }, section ".sealed_objects", align 8
@__const.thread_millisecond_wait.t = private unnamed_addr addrspace(200) constant %struct.Timeout { i32 0, i32 1 }, align 4
@llvm.compiler.used = appending addrspace(200) global [1 x ptr addrspace(200)] [ptr addrspace(200) @__default_malloc_capability], section "llvm.metadata"

; Function Attrs: minsize noreturn nounwind optsize
define dso_local cheriot_compartmentcalleecc void @_Z9do_thingsv() local_unnamed_addr addrspace(200) #0 {
entry:
  %t.i = alloca %struct.Timeout, align 4, addrspace(200)
  %call = notail call cheriot_compartmentcallcc ptr addrspace(200) @_Z13heap_allocateP7TimeoutU19__sealed_capabilityP24AllocatorCapabilityStatejj(ptr addrspace(200) noundef null, ptr addrspace(200) noundef null, i32 noundef 128, i32 noundef 0) #5
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %exitcond.not = icmp eq i32 %i.0, 32
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body

for.cond.cleanup:                                 ; preds = %for.cond
  %call1 = notail call cheriot_compartmentcallcc i32 @_Z9heap_freeU19__sealed_capabilityP24AllocatorCapabilityStatePv(ptr addrspace(200) noundef null, ptr addrspace(200) noundef %call) #5
  br label %while.cond

for.body:                                         ; preds = %for.cond
  %arrayidx = getelementptr inbounds nuw i32, ptr addrspace(200) %call, i32 %i.0
  store i32 %i.0, ptr addrspace(200) %arrayidx, align 4, !tbaa !6
  %inc = add nuw nsw i32 %i.0, 1
  br label %for.cond, !llvm.loop !10

while.cond:                                       ; preds = %while.cond, %for.cond.cleanup
  call void @llvm.lifetime.start.p200(i64 8, ptr addrspace(200) nonnull %t.i) #6
  call void @llvm.memcpy.p200.p200.i32(ptr addrspace(200) noundef nonnull align 4 dereferenceable(8) %t.i, ptr addrspace(200) noundef nonnull align 4 dereferenceable(8) @__const.thread_millisecond_wait.t, i32 8, i1 false)
  %call.i = notail call cheriot_compartmentcallcc i32 @_Z12thread_sleepP7Timeoutj(ptr addrspace(200) noundef nonnull %t.i, i32 noundef 0) #5
  call void @llvm.lifetime.end.p200(i64 8, ptr addrspace(200) nonnull %t.i) #6
  br label %while.cond
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p200(i64 immarg, ptr addrspace(200) captures(none)) addrspace(200) #1

; Function Attrs: minsize optsize
declare dso_local cheriot_compartmentcallcc ptr addrspace(200) @_Z13heap_allocateP7TimeoutU19__sealed_capabilityP24AllocatorCapabilityStatejj(ptr addrspace(200) noundef, ptr addrspace(200) noundef, i32 noundef, i32 noundef) local_unnamed_addr addrspace(200) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p200(i64 immarg, ptr addrspace(200) captures(none)) addrspace(200) #1

; Function Attrs: minsize optsize
declare dso_local cheriot_compartmentcallcc i32 @_Z9heap_freeU19__sealed_capabilityP24AllocatorCapabilityStatePv(ptr addrspace(200) noundef, ptr addrspace(200) noundef) local_unnamed_addr addrspace(200) #2

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p200.p200.i32(ptr addrspace(200) noalias writeonly captures(none), ptr addrspace(200) noalias readonly captures(none), i32, i1 immarg) addrspace(200) #3

; Function Attrs: minsize optsize
declare dso_local cheriot_compartmentcallcc i32 @_Z12thread_sleepP7Timeoutj(ptr addrspace(200) noundef, i32 noundef) local_unnamed_addr addrspace(200) #4

attributes #0 = { minsize noreturn nounwind optsize "cheri-compartment"="hello" "interrupt-state"="enabled" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot-ibex" "target-features"="+32bit,+b,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zba,+zbb,+zbkb,+zbkc,+zbs,+zca,+zmmul,-a,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zbc,-zbkx,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { minsize optsize "cheri-compartment"="allocator" "interrupt-state"="enabled" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot-ibex" "target-features"="+32bit,+b,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zba,+zbb,+zbkb,+zbkc,+zbs,+zca,+zmmul,-a,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zbc,-zbkx,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #3 = { mustprogress nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #4 = { minsize optsize "cheri-compartment"="scheduler" "interrupt-state"="disabled" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot-ibex" "target-features"="+32bit,+b,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zba,+zbb,+zbkb,+zbkc,+zbs,+zca,+zmmul,-a,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zbc,-zbkx,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #5 = { minsize nounwind optsize "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" }
attributes #6 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_b1p0_zmmul1p0_zca1p0_zba1p0_zbb1p0_zbkb1p0_zbkc1p0_zbs1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
!5 = !{!"clang version 21.1.8 (git@github.com:resistor/llvm-project-1.git 6979a6dd3c0e6b2ca1f48f17adfcc83e06d2b836)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = distinct !{!10, !11}
!11 = !{!"llvm.loop.mustprogress"}
