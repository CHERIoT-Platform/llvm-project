; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; CHECK-NOT: cincoffset	{{.*}}, sp, 16

%struct.__Sealed_____default_malloc_capability_type = type { i32, i32, %struct.AllocatorCapabilityState }
%struct.AllocatorCapabilityState = type { i32, i32, [2 x ptr addrspace(200)] }
%struct.Timeout = type { i32, i32 }

@.str.2 = external hidden unnamed_addr addrspace(200) constant [18 x i8], align 1
@.str.3 = external hidden unnamed_addr addrspace(200) constant [25 x i8], align 1
@__default_malloc_capability = external dso_local addrspace(200) global %struct.__Sealed_____default_malloc_capability_type, section ".sealed_objects", align 8 #0

define hidden fastcc void @_ZN12_GLOBAL__N_124test_sub_quota_semanticsEv() unnamed_addr addrspace(200) #1 {
for.cond5.preheader:
  %t = alloca %struct.Timeout, align 4, addrspace(200)
  %quotas = alloca [4 x ptr addrspace(200)], align 8, addrspace(200)
  call addrspace(200) void @llvm.lifetime.start.p200(ptr addrspace(200) nonnull %t) #6
  store i32 0, ptr addrspace(200) %t, align 4, !tbaa !10
  %remaining.i = getelementptr inbounds nuw i8, ptr addrspace(200) %t, i32 4
  store i32 -1, ptr addrspace(200) %remaining.i, align 4, !tbaa !12
  call addrspace(200) void @llvm.lifetime.start.p200(ptr addrspace(200) nonnull %quotas) #6
  store ptr addrspace(200) @__default_malloc_capability, ptr addrspace(200) %quotas, align 8, !tbaa !13
  %0 = getelementptr inbounds nuw i8, ptr addrspace(200) %quotas, i32 8
  %call = notail call cheriot_compartmentcallcc addrspace(200) ptr addrspace(200) @_Z15split_sub_quotaP7TimeoutU19__sealed_capabilityP24AllocatorCapabilityStatej(ptr addrspace(200) noundef nonnull %t, ptr addrspace(200) noundef nonnull @__default_malloc_capability, i32 noundef 528) #7
  store ptr addrspace(200) %call, ptr addrspace(200) %0, align 8, !tbaa !13
  %1 = getelementptr inbounds nuw i8, ptr addrspace(200) %quotas, i32 16
  %call.1 = notail call cheriot_compartmentcallcc addrspace(200) ptr addrspace(200) @_Z15split_sub_quotaP7TimeoutU19__sealed_capabilityP24AllocatorCapabilityStatej(ptr addrspace(200) noundef nonnull %t, ptr addrspace(200) noundef %call, i32 noundef 352) #7
  store ptr addrspace(200) %call.1, ptr addrspace(200) %1, align 8, !tbaa !13
  %2 = getelementptr inbounds nuw i8, ptr addrspace(200) %quotas, i32 24
  %call.2 = notail call cheriot_compartmentcallcc addrspace(200) ptr addrspace(200) @_Z15split_sub_quotaP7TimeoutU19__sealed_capabilityP24AllocatorCapabilityStatej(ptr addrspace(200) noundef nonnull %t, ptr addrspace(200) noundef %call.1, i32 noundef 176) #7
  store ptr addrspace(200) %call.2, ptr addrspace(200) %2, align 8, !tbaa !13
  br label %for.cond5

for.cond5:                                        ; preds = %for.cond5.preheader, %_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj15EEEtlA15_cLc65ELc108ELc108ELc111ELc99ELc97ELc116ELc111ELc114ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE9InvariantIJRjRiEEC2EbPKcS7_S8_NS_14SourceLocationE.exit
  %storemerge = phi i32 [ %sub15, %_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj15EEEtlA15_cLc65ELc108ELc108ELc111ELc99ELc97ELc116ELc111ELc114ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE9InvariantIJRjRiEEC2EbPKcS7_S8_NS_14SourceLocationE.exit ], [ 3, %for.cond5.preheader ]
  %cmp6.not = icmp eq i32 %storemerge, 0
  br i1 %cmp6.not, label %for.cond.cleanup7, label %for.body8

for.cond.cleanup7:                                ; preds = %for.cond5
  call addrspace(200) void @llvm.lifetime.end.p200(ptr addrspace(200) nonnull %quotas) #6
  call addrspace(200) void @llvm.lifetime.end.p200(ptr addrspace(200) nonnull %t) #6
  ret void

for.body8:                                        ; preds = %for.cond5
  %arrayidx9 = getelementptr inbounds nuw ptr addrspace(200), ptr addrspace(200) %quotas, i32 %storemerge
  %3 = load ptr addrspace(200), ptr addrspace(200) %arrayidx9, align 8, !tbaa !13
  %arrayidx11 = getelementptr i8, ptr addrspace(200) %arrayidx9, i32 -8
  %4 = load ptr addrspace(200), ptr addrspace(200) %arrayidx11, align 8, !tbaa !13
  %call12 = notail call cheriot_compartmentcallcc addrspace(200) i32 @_Z19recombine_sub_quotaU19__sealed_capabilityP24AllocatorCapabilityStateS0_(ptr addrspace(200) noundef %3, ptr addrspace(200) noundef %4) #7
  %cmp13 = icmp eq i32 %call12, 0
  br i1 %cmp13, label %_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj15EEEtlA15_cLc65ELc108ELc108ELc111ELc99ELc97ELc116ELc111ELc114ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE9InvariantIJRjRiEEC2EbPKcS7_S8_NS_14SourceLocationE.exit, label %if.then.i, !prof !16

if.then.i:                                        ; preds = %for.body8
  call fastcc addrspace(200) void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj15EEEtlA15_cLc65ELc108ELc108ELc111ELc99ELc97ELc116ELc111ELc114ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJjiEEEvPKcS8_S8_iS8_DpT_(ptr addrspace(200) noundef nonnull @.str.2, ptr addrspace(200) noundef nonnull @.str.3, i32 noundef 1509, i32 noundef %storemerge, i32 noundef %call12) #8
  call addrspace(200) void @llvm.trap()
  unreachable

_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj15EEEtlA15_cLc65ELc108ELc108ELc111ELc99ELc97ELc116ELc111ELc114ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE9InvariantIJRjRiEEC2EbPKcS7_S8_NS_14SourceLocationE.exit: ; preds = %for.body8
  %sub15 = add nsw i32 %storemerge, -1
  br label %for.cond5, !llvm.loop !17
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p200(ptr addrspace(200) captures(none)) addrspace(200) #2

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p200(ptr addrspace(200) captures(none)) addrspace(200) #2

; Function Attrs: minsize optsize
declare dso_local cheriot_compartmentcallcc ptr addrspace(200) @_Z15split_sub_quotaP7TimeoutU19__sealed_capabilityP24AllocatorCapabilityStatej(ptr addrspace(200) noundef, ptr addrspace(200) noundef, i32 noundef) local_unnamed_addr addrspace(200) #3

; Function Attrs: minsize optsize
declare dso_local cheriot_compartmentcallcc i32 @_Z19recombine_sub_quotaU19__sealed_capabilityP24AllocatorCapabilityStateS0_(ptr addrspace(200) noundef, ptr addrspace(200) noundef) local_unnamed_addr addrspace(200) #3

; Function Attrs: inlinehint minsize mustprogress nounwind optsize
declare hidden fastcc void @_ZN12_GLOBAL__N_116ConditionalDebugIXtlNS_26DebugLevelTemplateArgumentEEEXtlNS_12DebugContextILj15EEEtlA15_cLc65ELc108ELc108ELc111ELc99ELc97ELc116ELc111ELc114ELc32ELc116ELc101ELc115ELc116EEEELb1ELb1EE14report_failureIJjiEEEvPKcS8_S8_iS8_DpT_(ptr addrspace(200) noundef, ptr addrspace(200) noundef, i32 noundef, i32 noundef, i32 noundef) unnamed_addr addrspace(200) #4 align 2

; Function Attrs: cold noreturn nounwind memory(inaccessiblemem: write)
declare void @llvm.trap() addrspace(200) #5

attributes #0 = { "cheriot_sealed_value" }
attributes #1 = { minsize mustprogress noinline nounwind optsize "cheri-compartment"="allocator_test" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smpmpmt,-experimental-svukte,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zibi,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvfbfa,-experimental-zvfofp8min,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smctr,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssctr,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsinth,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipsexectl,-xmipslsp,-xqccmp,-xqci,-xqcia,-xqciac,-xqcibi,-xqcibm,-xqcicli,-xqcicm,-xqcics,-xqcicsr,-xqciint,-xqciio,-xqcilb,-xqcili,-xqcilia,-xqcilo,-xqcilsm,-xqcisim,-xqcisls,-xqcisync,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfbfexp16e,-xsfvfexp16e,-xsfvfexp32e,-xsfvfexpa,-xsfvfexpa64e,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xsmtvdot,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalasr,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #3 = { minsize optsize "cheri-compartment"="allocator" "interrupt-state"="enabled" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smpmpmt,-experimental-svukte,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zibi,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvfbfa,-experimental-zvfofp8min,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smctr,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssctr,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsinth,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipsexectl,-xmipslsp,-xqccmp,-xqci,-xqcia,-xqciac,-xqcibi,-xqcibm,-xqcicli,-xqcicm,-xqcics,-xqcicsr,-xqciint,-xqciio,-xqcilb,-xqcili,-xqcilia,-xqcilo,-xqcilsm,-xqcisim,-xqcisls,-xqcisync,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfbfexp16e,-xsfvfexp16e,-xsfvfexp32e,-xsfvfexpa,-xsfvfexpa64e,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xsmtvdot,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalasr,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #4 = { inlinehint minsize mustprogress nounwind optsize "cheri-compartment"="allocator_test" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smpmpmt,-experimental-svukte,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zibi,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvfbfa,-experimental-zvfofp8min,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smctr,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssctr,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsinth,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipsexectl,-xmipslsp,-xqccmp,-xqci,-xqcia,-xqciac,-xqcibi,-xqcibm,-xqcicli,-xqcicm,-xqcics,-xqcicsr,-xqciint,-xqciio,-xqcilb,-xqcili,-xqcilia,-xqcilo,-xqcilsm,-xqcisim,-xqcisls,-xqcisync,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfbfexp16e,-xsfvfexp16e,-xsfvfexp32e,-xsfvfexpa,-xsfvfexpa64e,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xsmtvdot,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalasr,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #5 = { cold noreturn nounwind memory(inaccessiblemem: write) }
attributes #6 = { nounwind }
attributes #7 = { minsize nounwind optsize "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" }
attributes #8 = { minsize optsize "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" }

!llvm.module.flags = !{!0, !1, !2, !4}
!llvm.ident = !{!5}
!llvm.errno.tbaa = !{!6}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_zca1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
!5 = !{!"clang version 22.1.4 (git@github.com:resistor/llvm-project-1.git 3d5dcbbd476e1402c109c6ff3b5f481e7ca00297)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C++ TBAA"}
!10 = !{!11, !7, i64 0}
!11 = !{!"_ZTS7Timeout", !7, i64 0, !7, i64 4}
!12 = !{!11, !7, i64 4}
!13 = !{!14, !14, i64 0}
!14 = !{!"p1 _ZTS24AllocatorCapabilityState", !15, i64 0}
!15 = !{!"any pointer", !8, i64 0}
!16 = !{!"branch_weights", !"expected", i32 2000, i32 1}
!17 = distinct !{!17, !18}
!18 = !{!"llvm.loop.mustprogress"}
