; RUN: opt < %s -loop-reduce -S | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-unknown"

;; Ensure that LSR does not create traversal starting at a negative initial offset,
;; as those are not representable on CHERIOT.

; CHECK-LABEL: @strlen
define dso_local cherilibcallcc i32 @strlen(ptr addrspace(200) noundef readonly %str) local_unnamed_addr addrspace(200) #0 {
; CHECK: entry:
; CHECK-NOT: -1
entry:
  br label %for.cond

; CHECK: for.cond:
for.cond:                                         ; preds = %for.cond, %entry
  %s.0 = phi ptr addrspace(200) [ %str, %entry ], [ %incdec.ptr, %for.cond ]
  %0 = load i8, ptr addrspace(200) %s.0, align 1, !tbaa !6
  %tobool.not = icmp eq i8 %0, 0
  %incdec.ptr = getelementptr inbounds nuw i8, ptr addrspace(200) %s.0, i32 1
  br i1 %tobool.not, label %for.end, label %for.cond, !llvm.loop !9

for.end:                                          ; preds = %for.cond
  %1 = tail call i32 @llvm.cheri.cap.diff.i32(ptr addrspace(200) nonnull %s.0, ptr addrspace(200) %str)
  ret i32 %1
}

; Function Attrs: mustprogress nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.cheri.cap.diff.i32(ptr addrspace(200), ptr addrspace(200)) addrspace(200) #1

attributes #0 = { minsize nofree nosync nounwind optsize memory(argmem: read) "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+zmmul,-a,-b,-d,-experimental-sdext,-experimental-sdtrig,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqcia,-experimental-xqciac,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisls,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-f,-h,-i,-sha,-shcounterenw,-shgatpa,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscmove,-xmipslsp,-xsfcease,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,+zca,-zcb,-zcd,-zce,-zcf,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { mustprogress nofree nosync nounwind willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriotrtos"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
!5 = !{!"clang version 20.1.3 (git@github.com:resistor/llvm-project-1.git bfb9e867619569023263b0c2418ca004603f3fd1)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10}
!10 = !{!"llvm.loop.mustprogress"}
