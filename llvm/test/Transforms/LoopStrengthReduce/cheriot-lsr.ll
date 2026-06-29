; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

@buf = dso_local local_unnamed_addr addrspace(200) global [10 x i8] zeroinitializer, align 1

; Verify that LSR does not hoist the address offset out of the inner loop.

; CHECK: f:
; CHECK: .LBB0_6:
; CHECK: auipcc.data
; CHECK: cincoffset
; CHECK: csetbounds
; CHECK-NOT: cincoffset
; CHECK: beqz
; CHECK: .LBB0_1:

; Function Attrs: minsize nofree norecurse nosync nounwind optsize memory(write, argmem: none, inaccessiblemem: none)
define dso_local void @f(i32 noundef %y_offset) local_unnamed_addr addrspace(200) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %y.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %exitcond.not = icmp eq i32 %y.0, 10
  br i1 %exitcond.not, label %cleanup, label %for.body

for.body:                                         ; preds = %for.cond
  %add = add nsw i32 %y.0, %y_offset
  %cmp1 = icmp slt i32 %add, 0
  br i1 %cmp1, label %for.inc, label %if.end

if.end:                                           ; preds = %for.body
  %cmp3 = icmp samesign ugt i32 %add, 9
  br i1 %cmp3, label %cleanup, label %if.end5

if.end5:                                          ; preds = %if.end
  %arrayidx = getelementptr inbounds nuw [10 x i8], ptr addrspace(200) @buf, i32 0, i32 %add
  store i8 1, ptr addrspace(200) %arrayidx, align 1, !tbaa !6
  br label %for.inc

for.inc:                                          ; preds = %for.body, %if.end5
  %inc = add nuw nsw i32 %y.0, 1
  br label %for.cond, !llvm.loop !9

cleanup:                                          ; preds = %if.end, %for.cond
  ret void
}

attributes #0 = { minsize nofree norecurse nosync nounwind optsize memory(write, argmem: none, inaccessiblemem: none) "cheri-compartment"="hello" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot-ibex" "target-features"="+32bit,+b,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+xcheripurecap,+zba,+zbb,+zbkb,+zbkc,+zbs,+zca,+zmmul,-a,-d,-experimental-p,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqccmp,-experimental-xqcia,-experimental-xqciac,-experimental-xqcibi,-experimental-xqcibm,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqciio,-experimental-xqcilb,-experimental-xqcili,-experimental-xqcilia,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisim,-experimental-xqcisls,-experimental-xqcisync,-experimental-xrivosvisni,-experimental-xrivosvizip,-experimental-xsfmclic,-experimental-xsfsclic,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-experimental-zvqdotq,-f,-h,-i,-q,-sdext,-sdtrig,-sha,-shcounterenw,-shgatpa,-shlcofideleg,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcntrpmf,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xandesbfhcvt,-xandesperf,-xandesvbfhcvt,-xandesvdot,-xandesvpackfph,-xandesvsintload,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscbop,-xmipscmov,-xmipslsp,-xsfcease,-xsfmm128t,-xsfmm16t,-xsfmm32a16f,-xsfmm32a32f,-xsfmm32a8f,-xsfmm32a8i,-xsfmm32t,-xsfmm64a64f,-xsfmm64t,-xsfmmbase,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zbc,-zbkx,-zcb,-zcd,-zce,-zcf,-zclsd,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccamoc,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zilsd,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }

!llvm.module.flags = !{!0, !1, !2, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_b1p0_zmmul1p0_zca1p0_zba1p0_zbb1p0_zbkb1p0_zbkc1p0_zbs1p0_xcheri0p0_xcheriot1p0_xcheripurecap0p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
!5 = !{!"clang version 21.1.8 (git@github.com:resistor/llvm-project-1.git fe4f0507c25d3634c556e334395d798ebc413eca)"}
!6 = !{!7, !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = distinct !{!9, !10}
!10 = !{!"llvm.loop.mustprogress"}
