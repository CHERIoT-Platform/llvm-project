; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-unknown"

; This test would previously try to take the index negatively out of bounds
; and then bring it back in with constant offsets.
; CHECK: do.end
; CHECK-NOT: neg
; CHECK: ccall alloc

define dso_local void @bad(i32 noundef %val) local_unnamed_addr addrspace(200) #0 {
entry:
  %buf = alloca [5 x i8], align 1, addrspace(200)
  call addrspace(200) void @llvm.lifetime.start.p200(ptr addrspace(200) nonnull %buf) #5
  br label %do.body

do.body:                                          ; preds = %do.body, %entry
  %val.addr.0 = phi i32 [ %val, %entry ], [ %shr, %do.body ]
  %p.0.idx = phi i32 [ 5, %entry ], [ %p.0.add, %do.body ]
  %0 = trunc i32 %val.addr.0 to i8
  %conv = and i8 %0, 127
  %p.0.add = add nsw i32 %p.0.idx, -1
  %1 = call addrspace(200) ptr addrspace(200) @llvm.cheri.bounded.stack.cap.i32(ptr addrspace(200) %buf, i32 5)
  %incdec.ptr.ptr = getelementptr inbounds i8, ptr addrspace(200) %1, i32 %p.0.add
  store i8 %conv, ptr addrspace(200) %incdec.ptr.ptr, align 1, !tbaa !10
  %shr = lshr i32 %val.addr.0, 7
  %cmp.not = icmp eq i32 %shr, 0
  br i1 %cmp.not, label %do.end, label %do.body, !llvm.loop !11

do.end:                                           ; preds = %do.body
  %2 = call addrspace(200) ptr addrspace(200) @llvm.cheri.bounded.stack.cap.i32(ptr addrspace(200) %buf, i32 5)
  %add.ptr.ptr = getelementptr inbounds nuw i8, ptr addrspace(200) %2, i32 5
  %3 = call addrspace(200) i32 @llvm.cheri.cap.diff.i32(ptr addrspace(200) nonnull %add.ptr.ptr, ptr addrspace(200) nonnull %incdec.ptr.ptr)
  %call = call addrspace(200) ptr addrspace(200) @alloc(i32 noundef %3) #6
  %tobool.not = icmp eq ptr addrspace(200) %call, null
  br i1 %tobool.not, label %if.end, label %while.cond.preheader

while.cond.preheader:                             ; preds = %do.end
  %cmp7.not20 = icmp eq i32 %p.0.add, 4
  br i1 %cmp7.not20, label %while.end, label %while.body.preheader

while.body.preheader:                             ; preds = %while.cond.preheader
  br label %while.body

while.body:                                       ; preds = %while.body.preheader, %while.body
  %p.1.ptr23 = phi ptr addrspace(200) [ %p.1.ptr, %while.body ], [ %incdec.ptr.ptr, %while.body.preheader ]
  %c.022 = phi ptr addrspace(200) [ %incdec.ptr10, %while.body ], [ %call, %while.body.preheader ]
  %p.1.idx21 = phi i32 [ %p.1.add, %while.body ], [ %p.0.add, %while.body.preheader ]
  %p.1.add = add nsw i32 %p.1.idx21, 1
  %4 = load i8, ptr addrspace(200) %p.1.ptr23, align 1, !tbaa !10
  %incdec.ptr10 = getelementptr inbounds nuw i8, ptr addrspace(200) %c.022, i32 1
  store i8 %4, ptr addrspace(200) %c.022, align 1, !tbaa !10
  %5 = call addrspace(200) ptr addrspace(200) @llvm.cheri.bounded.stack.cap.i32(ptr addrspace(200) %buf, i32 5)
  %p.1.ptr = getelementptr inbounds i8, ptr addrspace(200) %5, i32 %p.1.add
  %cmp7.not = icmp eq i32 %p.1.add, 4
  br i1 %cmp7.not, label %while.end.loopexit, label %while.body, !llvm.loop !13

while.end.loopexit:                               ; preds = %while.body
  br label %while.end

while.end:                                        ; preds = %while.end.loopexit, %while.cond.preheader
  %c.0.lcssa = phi ptr addrspace(200) [ %call, %while.cond.preheader ], [ %incdec.ptr10, %while.end.loopexit ]
  %p.1.ptr.lcssa = phi ptr addrspace(200) [ %incdec.ptr.ptr, %while.cond.preheader ], [ %p.1.ptr, %while.end.loopexit ]
  %6 = load i8, ptr addrspace(200) %p.1.ptr.lcssa, align 1, !tbaa !10
  store i8 %6, ptr addrspace(200) %c.0.lcssa, align 1, !tbaa !10
  br label %if.end

if.end:                                           ; preds = %while.end, %do.end
  call addrspace(200) void @llvm.lifetime.end.p200(ptr addrspace(200) nonnull %buf) #5
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p200(ptr addrspace(200) captures(none) %0) addrspace(200) #1

; Function Attrs: optsize
declare dso_local ptr addrspace(200) @alloc(i32 noundef %0) local_unnamed_addr addrspace(200) #2

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.cheri.cap.diff.i32(ptr addrspace(200) %0, ptr addrspace(200) %1) addrspace(200) #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p200(ptr addrspace(200) captures(none) %0) addrspace(200) #1

; Function Attrs: nounwind willreturn memory(none)
declare ptr addrspace(200) @llvm.cheri.bounded.stack.cap.i32(ptr addrspace(200) %0, i32 %1) addrspace(200) #4

attributes #0 = { nounwind optsize "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smpmpmt,-experimental-svukte,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zibi,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvfbfa,-experimental-zvfofp8min,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smctr,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssctr,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsinth,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipsexectl,-xmipslsp,-xqccmp,-xqci,-xqcia,-xqciac,-xqcibi,-xqcibm,-xqcicli,-xqcicm,-xqcics,-xqcicsr,-xqciint,-xqciio,-xqcilb,-xqcili,-xqcilia,-xqcilo,-xqcilsm,-xqcisim,-xqcisls,-xqcisync,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfbfexp16e,-xsfvfexp16e,-xsfvfexp32e,-xsfvfexpa,-xsfvfexpa64e,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xsmtvdot,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalasr,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { optsize "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zca,+zmmul,-a,-b,-d,-experimental-p,-experimental-smpmpmt,-experimental-svukte,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zibi,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvfbfa,-experimental-zvfofp8min,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smctr,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssctr,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsinth,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipsexectl,-xmipslsp,-xqccmp,-xqci,-xqcia,-xqciac,-xqcibi,-xqcibm,-xqcicli,-xqcicm,-xqcics,-xqcicsr,-xqciint,-xqciio,-xqcilb,-xqcili,-xqcilia,-xqcilo,-xqcilsm,-xqcisim,-xqcisls,-xqcisync,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfbfexp16e,-xsfvfexp16e,-xsfvfexp32e,-xsfvfexpa,-xsfvfexpa64e,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xsmtvdot,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalasr,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #3 = { mustprogress nofree nosync nounwind willreturn memory(none) }
attributes #4 = { nounwind willreturn memory(none) }
attributes #5 = { nounwind }
attributes #6 = { nobuiltin nounwind optsize "no-builtins" }

!llvm.module.flags = !{!0, !1, !2, !4}
!llvm.ident = !{!5}
!llvm.errno.tbaa = !{!6}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_zca1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
!5 = !{!"clang version 22.1.2 (git@github.com:resistor/llvm-project-1.git d356361bfe2f46d659b14efc283d55c1e10c84c0)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = !{!8, !8, i64 0}
!11 = distinct !{!11, !12}
!12 = !{!"llvm.loop.mustprogress"}
!13 = distinct !{!13, !12}