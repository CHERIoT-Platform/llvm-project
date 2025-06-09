; RUN: llc -O0 --filetype=asm --mcpu=cheriot --mtriple=riscv32cheriot-unknown-cheriotrtos -target-abi cheriot -mattr=+c,+xcheriot,+xcheri,+cap-mode < %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32cheriot-unknown-cheriotrtos"

;; The type of the to-be-sealed value.
%struct.TestType = type { i32 }

;; The shape of sealed `TestType`s.
%struct.SealedTestType = type { i32, i32, %struct.TestType }

%struct.AllocatorCapabilityState = type { i32, i32, [2 x ptr addrspace(200)] }

%struct.SealedAllocatorCapabilityState = type { i32, i32, %struct.AllocatorCapabilityState }

; Function Attrs: minsize mustprogress nounwind optsize
define dso_local chericcallcce noundef i32 @test_static_sealing() local_unnamed_addr addrspace(200) #0 {
entry:

;; CHECK:        .LBB0_1:                                # %entry
;; CHECK-NEXT:                                           # Label of block must be emitted
;; CHECK-NEXT:      auipcc	ca0, %cheriot_compartment_hi(__import.sealed_object.test)
;; CHECK-NEXT:      clc	ca0, %cheriot_compartment_lo_i(.LBB0_1)(ca0)
;; CHECK-NEXT:   .LBB0_3:                                # %entry
;; CHECK-NEXT:                                           # Label of block must be emitted
;; CHECK-NEXT:  	  auipcc  ct1, %cheriot_compartment_hi(__import_static_sealing_inner_test_static_sealed_object)
;; CHECK-NEXT:  	  clc	  ct1, %cheriot_compartment_lo_i(.LBB0_3)(ct1)
;; CHECK-NEXT:   .LBB0_2:                                # %entry
;; CHECK-NEXT:                                           # Label of block must be emitted
;; CHECK-NEXT:  	  auipcc  ct2, %cheriot_compartment_hi(.compartment_switcher)
;; CHECK-NEXT:  	  clc	  ct2, %cheriot_compartment_lo_i(.LBB0_2)(ct2)
;; CHECK-NEXT:  	  cjalr	  ct2
  %call = notail call chericcallcc noundef i32 @test_static_sealed_object(ptr addrspace(200) @test) #2
  %call2 = notail call chericcallcc noundef i32 @test_static_sealed_object(ptr addrspace(200) @__default_malloc_capability) #2
  ret i32 %call2
}

; Function Attrs: minsize optsize
declare dso_local chericcallcc noundef i32 @test_static_sealed_object(ptr addrspace(200)) local_unnamed_addr addrspace(200) #1

attributes #0 = { minsize mustprogress nounwind optsize "cheri-compartment"="static_sealing_test" "interrupt-state"="enabled" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+zmmul,-a,-b,-d,-experimental-sdext,-experimental-sdtrig,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqcia,-experimental-xqciac,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisls,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-f,-h,-i,-sha,-shcounterenw,-shgatpa,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscmove,-xmipslsp,-xsfcease,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zca,-zcb,-zcd,-zce,-zcf,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #1 = { minsize optsize "cheri-compartment"="static_sealing_inner" "interrupt-state"="enabled" "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+e,+m,+relax,+unaligned-scalar-mem,+xcheri,+xcheriot,+zmmul,-a,-b,-d,-experimental-sdext,-experimental-sdtrig,-experimental-smctr,-experimental-ssctr,-experimental-svukte,-experimental-xqcia,-experimental-xqciac,-experimental-xqcicli,-experimental-xqcicm,-experimental-xqcics,-experimental-xqcicsr,-experimental-xqciint,-experimental-xqcilo,-experimental-xqcilsm,-experimental-xqcisls,-experimental-zalasr,-experimental-zicfilp,-experimental-zicfiss,-experimental-zvbc32e,-experimental-zvkgs,-f,-h,-i,-sha,-shcounterenw,-shgatpa,-shtvala,-shvsatpa,-shvstvala,-shvstvecd,-smaia,-smcdeleg,-smcsrind,-smdbltrp,-smepmp,-smmpm,-smnpm,-smrnmi,-smstateen,-ssaia,-ssccfg,-ssccptr,-sscofpmf,-sscounterenw,-sscsrind,-ssdbltrp,-ssnpm,-sspm,-ssqosid,-ssstateen,-ssstrict,-sstc,-sstvala,-sstvecd,-ssu64xl,-supm,-svade,-svadu,-svbare,-svinval,-svnapot,-svpbmt,-svvptc,-v,-xcheri-norvc,-xcvalu,-xcvbi,-xcvbitmanip,-xcvelw,-xcvmac,-xcvmem,-xcvsimd,-xmipscmove,-xmipslsp,-xsfcease,-xsfvcp,-xsfvfnrclipxfqf,-xsfvfwmaccqqq,-xsfvqmaccdod,-xsfvqmaccqoq,-xsifivecdiscarddlone,-xsifivecflushdlone,-xtheadba,-xtheadbb,-xtheadbs,-xtheadcmo,-xtheadcondmov,-xtheadfmemidx,-xtheadmac,-xtheadmemidx,-xtheadmempair,-xtheadsync,-xtheadvdot,-xventanacondops,-xwchc,-za128rs,-za64rs,-zaamo,-zabha,-zacas,-zalrsc,-zama16b,-zawrs,-zba,-zbb,-zbc,-zbkb,-zbkc,-zbkx,-zbs,-zca,-zcb,-zcd,-zce,-zcf,-zcmop,-zcmp,-zcmt,-zdinx,-zfa,-zfbfmin,-zfh,-zfhmin,-zfinx,-zhinx,-zhinxmin,-zic64b,-zicbom,-zicbop,-zicboz,-ziccamoa,-ziccif,-zicclsm,-ziccrse,-zicntr,-zicond,-zicsr,-zifencei,-zihintntl,-zihintpause,-zihpm,-zimop,-zk,-zkn,-zknd,-zkne,-zknh,-zkr,-zks,-zksed,-zksh,-zkt,-ztso,-zvbb,-zvbc,-zve32f,-zve32x,-zve64d,-zve64f,-zve64x,-zvfbfmin,-zvfbfwma,-zvfh,-zvfhmin,-zvkb,-zvkg,-zvkn,-zvknc,-zvkned,-zvkng,-zvknha,-zvknhb,-zvks,-zvksc,-zvksed,-zvksg,-zvksh,-zvkt,-zvl1024b,-zvl128b,-zvl16384b,-zvl2048b,-zvl256b,-zvl32768b,-zvl32b,-zvl4096b,-zvl512b,-zvl64b,-zvl65536b,-zvl8192b" }
attributes #2 = { minsize nounwind optsize "no-builtin-longjmp" "no-builtin-printf" "no-builtin-setjmp" }

!llvm.linker.options = !{}
!llvm.module.flags = !{!0, !1, !2, !4}

@__export.sealing_type.static_sealing_inner.SealingType = external dso_local addrspace(200) global i32, align 4
@__export.sealing_type.allocator.MallocKey = external dso_local addrspace(200) global i32, align 4

$test = comdat any
$__default_malloc_capability = comdat any
;; CHECK: 	.type	test,@object                    # @test
;; CHECK-NEXT: 	.section	.sealed_objects,"awG",@progbits,test,comdat
;; CHECK-NEXT: 	.weak	test
;; CHECK-NEXT: 	.p2align	2, 0x0
;; CHECK-NEXT: test:
;; CHECK-NEXT: 	.word	__export.sealing_type.static_sealing_inner.SealingType
;; CHECK-NEXT: 	.word	0                               # 0x0
;; CHECK-NEXT: 	.word	42                              # 0x2a
;; CHECK-NEXT: 	.size	test, 12
@test = linkonce_odr dso_local addrspace(200) global %struct.SealedTestType { i32 ptrtoint (ptr addrspace(200) @__export.sealing_type.static_sealing_inner.SealingType to i32), i32 0, %struct.TestType {i32 42}}, section ".sealed_objects", comdat, align 4 "cheriot_sealed_value"
;; CHECK: 	.type	__default_malloc_capability,@object                    # @__default_malloc_capability
;; CHECK-NEXT: 	.section	.sealed_objects,"awG",@progbits,__default_malloc_capability,comdat
;; CHECK-NEXT: 	.weak	__default_malloc_capability
;; CHECK-NEXT: 	.p2align	3, 0x0
;; CHECK-NEXT:  __default_malloc_capability:
;; CHECK-NEXT:  .word   __export.sealing_type.allocator.MallocKey
;; CHECK-NEXT:  .word   0                               # 0x0
;; CHECK-NEXT:  .word   1048576                         # 0x100000
;; CHECK-NEXT:  .word   0                               # 0x0
;; CHECK-NEXT:  .zero   16
;; CHECK-NEXT:  .size   __default_malloc_capability, 32
@__default_malloc_capability = linkonce_odr dso_local addrspace(200) global
%struct.SealedAllocatorCapabilityState { i32 ptrtoint (ptr
addrspace(200) @__export.sealing_type.allocator.MallocKey to i32), i32 0,
%struct.AllocatorCapabilityState { i32 1048576, i32 0, [2 x ptr addrspace(200)]
zeroinitializer } }, section ".sealed_objects", comdat, align 8 "cheriot_sealed_value"

@llvm.compiler.used = appending addrspace(200) global [2 x ptr addrspace(200)] [ptr addrspace(200) @test, ptr addrspace(200) @__default_malloc_capability], section "llvm.metadata"


;; The use of @test above must generate this import as well.

;; CHECK:	  .section .compartment_imports.test,"awG",@progbits,__import.sealed_object.test
;; CHECK-NEXT:	  .type	  __import.sealed_object.test,@object
;; CHECK-NEXT:	  .weak	  __import.sealed_object.test
;; CHECK-NEXT:	  .p2align 3, 0x0
;; CHECK-NEXT:  __import.sealed_object.test:
;; CHECK-NEXT:	  .word	test
;; CHECK-NEXT:	  .word	12
;; CHECK-NEXT:	  .size	__import.sealed_object.test, 8
;; CHECK:         .section .compartment_imports.__default_malloc_capability,"awG",@progbits,__import.sealed_object.__default_malloc_capability
;; CHECK-NEXT:	  .type	  __import.sealed_object.__default_malloc_capability,@object
;; CHECK-NEXT:	  .weak	  __import.sealed_object.__default_malloc_capability
;; CHECK-NEXT:	  .p2align 3, 0x0
;; CHECK-NEXT:  __import.sealed_object.__default_malloc_capability:
;; CHECK-NEXT:	  .word	__default_malloc_capability
;; CHECK-NEXT:	  .word
;; CHECK-NEXT:	  .size	__import.sealed_object.__default_malloc_capability, 8


!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0_xcheriot1p0"}
!4 = !{i32 8, !"SmallDataLimit", i32 0}
