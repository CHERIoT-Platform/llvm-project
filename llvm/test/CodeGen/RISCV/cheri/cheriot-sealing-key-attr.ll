; RUN: llc -O0 --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown-cheriotrtos -target-abi cheriot -mattr=+xcheri,+cap-mode < %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos-unknown"

@__import.sealing_type.static_sealing_test.SealingType= external  addrspace(200) constant ptr #0

; Function Attrs: noinline nounwind optnone
define dso_local void @func() #1 {
entry:

  %SealingKey = alloca ptr, align 8, addrspace(200)
; CHECK:  .LBB0_1:                                # %entry
; CHECK:                                          # Label of block must be emitted
; CHECK:      auipcc  ca0, %cheriot_compartment_hi(__import.sealing_type.static_sealing_test.SealingType)
; CHECK:      clc     ca0, %cheriot_compartment_lo_i(.LBB0_1)
  store ptr addrspace(200) @__import.sealing_type.static_sealing_test.SealingType, ptr addrspace(200) %SealingKey, align 8
  ret void
}

attributes #0 = { "cheri-compartment"="static_sealing_test" "cheriot_sealing_key"="sealing_type.static_sealing_test.SealingType" }
attributes #1 = { noinline nounwind optnone "cheri-compartment"="static_sealing_test" "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic-rv32" "target-features"="+32bit,+a,+c,+d,+f,+m,+relax,+zaamo,+zalrsc,+zicsr,+zmmul,-b,-e,-experimental-sdext,-experimental-sdtrig,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqcia,-experimental-xqciac,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisls,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-h,-sha,-shcounterenw,-shgatpa,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xcheri,-xcheriot,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscmove,-xmipslsp,-xsfcease,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zabha,-zacas,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zca,-zcb,-zcd,-zce,-zcf,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zifencei,-zihintntl,-zihintpause,-zihpm,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }



; CHECK:	.section .compartment_exports.sealing_type.static_sealing_test.SealingType,"awG",@progbits,sealing_type.static_sealing_test.SealingType,comdat
; CHECK:	.type	__export.sealing_type.static_sealing_test.SealingType,@object
; CHECK:	.globl	__export.sealing_type.static_sealing_test.SealingType
; CHECK:	.p2align	2, 0x0
; CHECK:__export.sealing_type.static_sealing_test.SealingType:
; CHECK:	.half	0
; CHECK:	.byte	0
; CHECK:	.byte	32
; CHECK:	.size	__export.sealing_type.static_sealing_test.SealingType, 4
; CHECK:	.section .compartment_imports.sealing_type.static_sealing_test.SealingType,"awG",@progbits,__import.sealing_type.static_sealing_test.SealingType,comdat
; CHECK:	.type	__import.sealing_type.static_sealing_test.SealingType,@object
; CHECK:	.globl	__import.sealing_type.static_sealing_test.SealingType
; CHECK:	.p2align	3, 0x0
; CHECK:__import.sealing_type.static_sealing_test.SealingType:
; CHECK:	.word	__export.sealing_type.static_sealing_test.SealingType
; CHECK:	.word	0



!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
