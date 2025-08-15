; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32cheriot-unknown-cheriotrtos -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap,+xcheriot -o - | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32cheriot-unknown-cheriotrtos"

%struct.TwoIntegers = type { i32, i32 }
%struct.TwoPointers = type { ptr addrspace(200), ptr addrspace(200) }
%struct.InnerPtr = type { ptr addrspace(200) }
%struct.ParentPtr = type { ptr addrspace(200), %struct.InnerPtr }
%struct.PointerAndInt = type { ptr addrspace(200), i32 }

@dummy = internal unnamed_addr addrspace(200) global i32 0, align 4
@_Z9CheckIntsv.x = internal addrspace(200) global %struct.TwoIntegers zeroinitializer, align 4
@dummies = internal addrspace(200) global [5 x i32] [i32 1, i32 2, i32 3, i32 4, i32 5], align 4
@force_use = internal addrspace(200) global ptr addrspace(200) null, align 8
@_Z9CheckPtrsv.x = internal addrspace(200) global %struct.TwoPointers zeroinitializer, align 8
@_Z11CheckPtrIntv.x = internal addrspace(200) global { ptr addrspace(200), i32, [4 x i8] } zeroinitializer, align 8
@_Z14CheckParentPtrv.x = internal addrspace(200) global %struct.ParentPtr zeroinitializer, align 8
@llvm.compiler.used = appending addrspace(200) global [5 x ptr addrspace(200)] [ptr addrspace(200) @_Z11CheckPtrIntv.x, ptr addrspace(200) @_Z14CheckParentPtrv.x, ptr addrspace(200) @_Z9CheckIntsv.x, ptr addrspace(200) @_Z9CheckPtrsv.x, ptr addrspace(200) @force_use], section "llvm.metadata"

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, argmem: none, inaccessiblemem: none)
define dso_local chericcallcce i32 @_Z8GetValuev() local_unnamed_addr addrspace(200) #0 {
entry:
  %0 = load i32, ptr addrspace(200) @dummy, align 4, !tbaa !7
  %inc = add i32 %0, 1
  store i32 %inc, ptr addrspace(200) @dummy, align 4, !tbaa !7
  ret i32 %inc
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce [2 x i32] @_Z8InitIntsv() local_unnamed_addr addrspace(200) #1 {

;; CHECK:  _Z8InitIntsv:                           # @_Z8InitIntsv
entry:

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  ;; CHECK:  	mv	s0, a0
  %call = tail call chericcallcce i32 @_Z8GetValuev()

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  ;; CHECK:  	mv	a1, a0
  ;; CHECK:  	mv	a0, s0
  %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  %.fca.0.insert = insertvalue [2 x i32] poison, i32 %call, 0
  %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %call1, 1

  ;; Usual callee epilogue.
  ;; CHECK-NEXT:  	ct.clc	cra, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK-NEXT:  	ct.clc	cs0, 0(csp)                     # 8-byte Folded Reload
  ;; CHECK-NEXT:  	ct.cincoffset	csp, csp, 16
  ;; CHECK-NEXT:  	ct.cret
  ret [2 x i32] %.fca.1.insert
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce [2 x i32] @_Z7ChgInts11TwoIntegers([2 x i32] %x.coerce) local_unnamed_addr addrspace(200) #1 {

;; CHECK:  _Z7ChgInts11TwoIntegers:                # @_Z7ChgInts11TwoIntegers
entry:
  ;; CHECK:  	mv	s0, a1
  ;; CHECK:  	mv	s1, a0
  %x.coerce.fca.0.extract = extractvalue [2 x i32] %x.coerce, 0
  %x.coerce.fca.1.extract = extractvalue [2 x i32] %x.coerce, 1

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()

  ;; CHECK:  	sub	s1, s1, a0
  %sub = sub i32 %x.coerce.fca.0.extract, %call

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call1 = tail call chericcallcce i32 @_Z8GetValuev()

  ;; CHECK:  	sub	a1, s0, a0
  %sub2 = sub i32 %x.coerce.fca.1.extract, %call1

  %.fca.0.insert = insertvalue [2 x i32] poison, i32 %sub, 0
  %.fca.1.insert = insertvalue [2 x i32] %.fca.0.insert, i32 %sub2, 1

  ;; CHECK:  	mv	a0, s1
  ;; CHECK:  	ct.clc	cra, 24(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	cs0, 16(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	cs1, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK:  	ct.cincoffset	csp, csp, 32
  ;; CHECK:  	ct.cret
  ret [2 x i32] %.fca.1.insert
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce void @_Z9CheckIntsv() local_unnamed_addr addrspace(200) #2 {
entry:
  %call = tail call chericcallcce [2 x i32] @_Z8InitIntsv()
  %call1 = tail call chericcallcce [2 x i32] @_Z7ChgInts11TwoIntegers([2 x i32] %call)

  ;; CHECK: _Z9CheckIntsv:                          # @_Z9CheckIntsv
  ;; CHECK:	        ct.ccall	_Z8InitIntsv
  ;; CHECK-NEXT:	ct.ccall	_Z7ChgInts11TwoIntegers

  %call1.fca.0.extract = extractvalue [2 x i32] %call1, 0
  %call1.fca.1.extract = extractvalue [2 x i32] %call1, 1
  store i32 %call1.fca.0.extract, ptr addrspace(200) @_Z9CheckIntsv.x, align 4, !tbaa !7
  store i32 %call1.fca.1.extract, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z9CheckIntsv.x, i32 4), align 4, !tbaa !7
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p200(i64 immarg, ptr addrspace(200) nocapture) addrspace(200) #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p200(i64 immarg, ptr addrspace(200) nocapture) addrspace(200) #3

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.TwoPointers @_Z8InitPtrsv() local_unnamed_addr addrspace(200) #1 {
;; CHECK: _Z8InitPtrsv:                           # @_Z8InitPtrsv
entry:

  ;; CHECK:	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()

  ;; CHECK:     auicgp	        cs0, %cheriot_compartment_hi(dummies)
  ;; CHECK:     cincoffset	cs0, cs0, %cheriot_compartment_lo_i(.LBB4_1)
  ;; CHECK:     ct.csetbounds   cs0, cs0, %cheriot_compartment_size(dummies)
  ;; CHECK:     ct.cincoffset   ca0, cs0, a0
  %rem = urem i32 %call, 5
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK:	ct.ccall	_Z8GetValuev
  ;; CHECK:	ct.cincoffset	ca1, cs0, a0
  %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  %rem2 = urem i32 %call1, 5
  %add.ptr3 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem2
  %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr3, 1

  ;; CHECK:     ct.clc	ca0, 0(csp)                     # 8-byte Folded Reload
  ;; CHECK:	ct.clc	cra, 24(csp)                    # 8-byte Folded Reload
  ;; CHECK:	ct.clc	cs0, 16(csp)                    # 8-byte Folded Reload
  ;; CHECK:	ct.clc	cs1, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK:	ct.cincoffset	csp, csp, 32
  ;; CHECK:	ct.cret
  ret %struct.TwoPointers %.fca.1.insert
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.TwoPointers @_Z7ChgPtrs11TwoPointers(ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1) local_unnamed_addr addrspace(200) #1 {
;; CHECK: _Z7ChgPtrs11TwoPointers:                # @_Z7ChgPtrs11TwoPointers
;; CHECK: ct.csc	ca1, 16(csp)                    # 8-byte Folded Spill
entry:

  ;; CHECK: auicgp	ca1, %cheriot_compartment_hi(force_use)
  ;; CHECK: cincoffset	ca1, ca1, %cheriot_compartment_lo_i(.LBB5_1)
  ;; CHECK: ct.csc	ca1, 8(csp)                     # 8-byte Folded Spill
  ;; CHECK: ct.csc	ca0, 0(ca1)
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK: ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %rem = urem i32 %call, 5

  ;; %call above was saved/reloaded in a0
  ;; CHECK: auicgp	  cs0, %cheriot_compartment_hi(dummies)
  ;; CHECK: cincoffset	  cs0, cs0, %cheriot_compartment_lo_i(.LBB5_2)
  ;; CHECK: ct.csetbounds cs0, cs0, %cheriot_compartment_size(dummies)
  ;; CHECK: sub	          a0, a0, a1
  ;; CHECK: slli	  a0, a0, 2
  ;; CHECK: ct.cincoffset ca0, cs0, a0
  ;; CHECK: ct.csc	ca0, 0(csp)                     # 8-byte Folded Spill
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK: ct.clc  ca0, 16(csp)                    # 8-byte Folded Reload
  ;; CHECK: ct.clc  ca1, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK: ct.csc  ca0, 0(ca1)
  store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK: ct.ccall	_Z8GetValuev
  %call2 = tail call chericcallcce i32 @_Z8GetValuev()
  %rem3 = urem i32 %call2, 5

  ;; CHECK: ct.cincoffset	ca1, cs0, a0
  %add.ptr4 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem3

  ;; ca1 already contains the final value; reload ca0
  ;; CHECK: ct.clc	ca0, 0(csp)                     # 8-byte Folded Reload
  %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr4, 1

  ;; CHECK: ct.cret
  ret %struct.TwoPointers %.fca.1.insert
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce void @_Z9CheckPtrsv() local_unnamed_addr addrspace(200) #2 {
;; CHECK: _Z9CheckPtrsv:                          # @_Z9CheckPtrsv
entry:

  ;; CHECK:       ct.ccall	_Z8InitPtrsv
  ;; CHECK-NEXT:  ct.ccall	_Z7ChgPtrs11TwoPointers
  %call = tail call chericcallcce %struct.TwoPointers @_Z8InitPtrsv()
  %0 = extractvalue %struct.TwoPointers %call, 0
  %1 = extractvalue %struct.TwoPointers %call, 1
  %call1 = tail call chericcallcce %struct.TwoPointers @_Z7ChgPtrs11TwoPointers(ptr addrspace(200) %0, ptr addrspace(200) %1)
  %2 = extractvalue %struct.TwoPointers %call1, 0
  %3 = extractvalue %struct.TwoPointers %call1, 1
  store ptr addrspace(200) %2, ptr addrspace(200) @_Z9CheckPtrsv.x, align 8, !tbaa !11
  store ptr addrspace(200) %3, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z9CheckPtrsv.x, i32 8), align 8, !tbaa !11
  ret void
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.PointerAndInt @_Z10InitPtrIntv() local_unnamed_addr addrspace(200) #1 {

;; CHECK: _Z10InitPtrIntv:                        # @_Z10InitPtrIntv
entry:
  ;; CHECK: ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %rem = urem i32 %call, 5

  ;; CHECK:  	auicgp	ca1, %cheriot_compartment_hi(dummies)
  ;; CHECK:  	cincoffset	ca1, ca1, %cheriot_compartment_lo_i(.LBB7_1)
  ;; CHECK:  	ct.csetbounds	ca1, ca1, %cheriot_compartment_size(dummies)
  ;; CHECK:  	ct.cincoffset	cs0, ca1, a0
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call1 = tail call chericcallcce i32 @_Z8GetValuev()

  ;; CHECK:  	mv	  a1, a0
  ;; CHECK:  	ct.cmove  ca0, cs0
  ;; CHECK:  	ct.clc	  cra, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	  cs0, 0(csp)                     # 8-byte Folded Reload
  ;; CHECK:  	ct.cincoffset	csp, csp, 16
  %.fca.0.insert = insertvalue %struct.PointerAndInt poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.insert = insertvalue %struct.PointerAndInt %.fca.0.insert, i32 %call1, 1

  ;; CHECK:  	ct.cret
  ret %struct.PointerAndInt %.fca.1.insert
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.PointerAndInt @_Z9ChgPtrInt13PointerAndInt(ptr addrspace(200) %x.coerce0, i32 %x.coerce1) local_unnamed_addr addrspace(200) #1 {
;; CHECK: _Z9ChgPtrInt13PointerAndInt:            # @_Z9ChgPtrInt13PointerAndInt
;; CHECK:     mv	s0, a1
entry:

  ;; CHECK:  .LBB8_1:                                # %entry
  ;; CHECK:                                          # Label of block must be emitted
  ;; CHECK:  	auicgp	ca1, %cheriot_compartment_hi(force_use)
  ;; CHECK:  	cincoffset	ca1, ca1, %cheriot_compartment_lo_i(.LBB8_1)
  ;; CHECK:       ct.csc	ca0, 0(ca1)
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %rem = urem i32 %call, 5

  ;; CHECK:     auicgp	ca1, %cheriot_compartment_hi(dummies)
  ;; CHECK:     cincoffset	ca1, ca1, %cheriot_compartment_lo_i(.LBB8_2)
  ;; CHECK:  	ct.csetbounds	ca1, ca1, %cheriot_compartment_size(dummies)
  ;; CHECK:  	ct.cincoffset	cs1, ca1, a0
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK:	ct.ccall	_Z8GetValuev
  %call2 = tail call chericcallcce i32 @_Z8GetValuev()

  ;; CHECK:       sub	a1, s0, a0
  %sub = sub i32 %x.coerce1, %call2

  ;; CHECK:  	ct.cmove	ca0, cs1
  ;; CHECK:  	ct.clc	cra, 24(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	cs0, 16(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	cs1, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK:  	ct.cincoffset	csp, csp, 32
  %.fca.0.insert = insertvalue %struct.PointerAndInt poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.insert = insertvalue %struct.PointerAndInt %.fca.0.insert, i32 %sub, 1

  ;; CHECK:  	ct.cret
  ret %struct.PointerAndInt %.fca.1.insert
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce void @_Z11CheckPtrIntv() local_unnamed_addr addrspace(200) #2 {
;; CHECK:  _Z11CheckPtrIntv:                       # @_Z11CheckPtrIntv
entry:

  ;; CHECK:  	  ct.ccall	_Z10InitPtrIntv
  ;; CHECK-NEXT:  ct.ccall	_Z9ChgPtrInt13PointerAndInt
  %call = tail call chericcallcce %struct.PointerAndInt @_Z10InitPtrIntv()
  %0 = extractvalue %struct.PointerAndInt %call, 0
  %1 = extractvalue %struct.PointerAndInt %call, 1
  %call1 = tail call chericcallcce %struct.PointerAndInt @_Z9ChgPtrInt13PointerAndInt(ptr addrspace(200) %0, i32 %1)
  %2 = extractvalue %struct.PointerAndInt %call1, 0
  %3 = extractvalue %struct.PointerAndInt %call1, 1
  store ptr addrspace(200) %2, ptr addrspace(200) @_Z11CheckPtrIntv.x, align 8, !tbaa !11
  store i32 %3, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z11CheckPtrIntv.x, i32 8), align 8, !tbaa !7
  ret void
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.ParentPtr @_Z13InitParentPtrv() local_unnamed_addr addrspace(200) #1 {


;; CHECK: _Z13InitParentPtrv:                     # @_Z13InitParentPtrv
entry:

  ;; CHECK: 	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %rem = urem i32 %call, 5

  ;; CHECK: 	auicgp	cs0, %cheriot_compartment_hi(dummies)
  ;; CHECK:     cincoffset	cs0, cs0, %cheriot_compartment_lo_i(.LBB10_1)
  ;; CHECK: 	ct.csetbounds	cs0, cs0, %cheriot_compartment_size(dummies)
  ;; CHECK: 	ct.cincoffset	ca0, cs0, a0
  ;; CHECK: 	ct.csc	ca0, 0(csp)                     # 8-byte Folded Spill
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK: 	ct.ccall	_Z8GetValuev
  %call1 = tail call chericcallcce i32 @_Z8GetValuev()
  %rem2 = urem i32 %call1, 5

  ;; CHECK: 	ct.cincoffset	ca1, cs0, a0
  %add.ptr3 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem2

  ;; CHECK: 	ct.clc	ca0, 0(csp)                     # 8-byte Folded Reload
  %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr3, 1, 0

  ;; CHECK: 	ct.cret
  ret %struct.ParentPtr %.fca.1.0.insert
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.ParentPtr @_Z12ChgParentPtr9ParentPtr(ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1) local_unnamed_addr addrspace(200) #1 {

;; CHECK: _Z12ChgParentPtr9ParentPtr:             # @_Z12ChgParentPtr9ParentPtr
;; CHECK: 	ct.csc	ca1, 16(csp)                    # 8-byte Folded Spill
entry:

  ;; CHECK: 	auicgp	ca1, %cheriot_compartment_hi(force_use)
  ;; CHECK: 	cincoffset	ca1, ca1, %cheriot_compartment_lo_i(.LBB11_1)
  ;; CHECK: 	ct.csc	ca1, 8(csp)                     # 8-byte Folded Spill
  ;; CHECK: 	ct.csc	ca0, 0(ca1)
  %x.coerce1.fca.0.extract = extractvalue %struct.InnerPtr %x.coerce1, 0
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK: 	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %rem = urem i32 %call, 5

  ;; CHECK: 	auicgp	cs0, %cheriot_compartment_hi(dummies)
  ;; CHECK: 	cincoffset	cs0, cs0, %cheriot_compartment_lo_i(.LBB11_2)
  ;; CHECK: 	ct.csetbounds	cs0, cs0, %cheriot_compartment_size(dummies)
  ;; CHECK: 	ct.cincoffset	ca0, cs0, a0
  ;; CHECK: 	ct.csc	ca0, 0(csp)                     # 8-byte Folded Spill
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK: 	ct.clc	ca0, 16(csp)                    # 8-byte Folded Reload
  ;; CHECK: 	ct.clc	ca1, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK: 	ct.csc	ca0, 0(ca1)
  store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK: 	ct.ccall	_Z8GetValuev
  %call3 = tail call chericcallcce i32 @_Z8GetValuev()
  %rem4 = urem i32 %call3, 5

  ;; CHECK: 	ct.cincoffset	ca1, cs0, a0
  %add.ptr5 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem4

  ;; CHECK: 	ct.clc	ca0, 0(csp)                     # 8-byte Folded Reload
  %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr5, 1, 0

  ;; CHECK: 	ct.cret
  ret %struct.ParentPtr %.fca.1.0.insert
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce void @_Z14CheckParentPtrv() local_unnamed_addr addrspace(200) #2 {

;; CHECK: _Z14CheckParentPtrv:                    # @_Z14CheckParentPtrv
entry:

  ;; CHECK: 	 ct.ccall	_Z13InitParentPtrv
  ;; CHECK-NEXT: ct.ccall	_Z12ChgParentPtr9ParentPtr
  %call = tail call chericcallcce %struct.ParentPtr @_Z13InitParentPtrv()
  %0 = extractvalue %struct.ParentPtr %call, 0
  %1 = extractvalue %struct.ParentPtr %call, 1
  %call1 = tail call chericcallcce %struct.ParentPtr @_Z12ChgParentPtr9ParentPtr(ptr addrspace(200) %0, %struct.InnerPtr %1)
  %2 = extractvalue %struct.ParentPtr %call1, 0
  %3 = extractvalue %struct.ParentPtr %call1, 1
  %.fca.0.extract3 = extractvalue %struct.InnerPtr %3, 0
  store ptr addrspace(200) %2, ptr addrspace(200) @_Z14CheckParentPtrv.x, align 8, !tbaa !11
  store ptr addrspace(200) %.fca.0.extract3, ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @_Z14CheckParentPtrv.x, i32 8), align 8, !tbaa !11
  ret void
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.TwoPointers @_Z8ChgPtrs2i11TwoPointers(i32 noundef %new_int, ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1) local_unnamed_addr addrspace(200) #1 {

;; CHECK:  _Z8ChgPtrs2i11TwoPointers:              # @_Z8ChgPtrs2i11TwoPointers
;; CHECK:       ct.csc	ca2, 32(csp)                    # 8-byte Folded Spill
;; CHECK:  	ct.csw	a0, 28(csp)                     # 4-byte Folded Spill
entry:

  ;; CHECK:  	auicgp	ca0, %cheriot_compartment_hi(force_use)
  ;; CHECK:  	cincoffset	ca0, ca0, %cheriot_compartment_lo_i(.LBB13_1)
  ;; CHECK:  	ct.csc	ca0, 16(csp)                    # 8-byte Folded Spill
  ;; CHECK:  	ct.csc	ca1, 0(ca0)
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %add = add i32 %call, %new_int
  %rem = urem i32 %add, 5

  ;; CHECK:  	auicgp	cs0, %cheriot_compartment_hi(dummies)
  ;; CHECK:  	cincoffset	cs0, cs0, %cheriot_compartment_lo_i(.LBB13_2)
  ;; CHECK:  	ct.csetbounds	cs0, cs0, %cheriot_compartment_size(dummies)
  ;; CHECK:  	ct.cincoffset	ca0, cs0, a0
  ;; CHECK:  	ct.csc	ca0, 8(csp)                     # 8-byte Folded Spill
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK:  	ct.clc	ca0, 32(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	ca1, 16(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.csc	ca0, 0(ca1)
  store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  ;; CHECK:  	ct.clw	a1, 28(csp)                     # 4-byte Folded Reload
  %call2 = tail call chericcallcce i32 @_Z8GetValuev()
  %add3 = add i32 %call2, %new_int
  %rem4 = urem i32 %add3, 5

  ;; CHECK:  	ct.cincoffset	ca1, cs0, a0
  %add.ptr5 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem4

  ;; CHECK:  	ct.clc	ca0, 8(csp)                     # 8-byte Folded Reload
  %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr5, 1

  ;; CHECK:  	ct.cret
  ret %struct.TwoPointers %.fca.1.insert
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.ParentPtr @_Z13ChgParentPtr2i9ParentPtr(i32 noundef %new_int, ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1) local_unnamed_addr addrspace(200) #1 {

;; CHECK:  _Z13ChgParentPtr2i9ParentPtr:           # @_Z13ChgParentPtr2i9ParentPtr
;; CHECK:  	ct.csc	ca2, 32(csp)                    # 8-byte Folded Spill
;; CHECK:  	mv	s0, a0
;; CHECK:  	ct.csw	a0, 28(csp)                     # 4-byte Folded Spill
entry:

  ;; CHECK:  	auicgp	ca0, %cheriot_compartment_hi(force_use)
  ;; CHECK:  	cincoffset	ca0, ca0, %cheriot_compartment_lo_i(.LBB14_1)
  ;; CHECK:  	ct.csc	ca0, 16(csp)                    # 8-byte Folded Spill
  ;; CHECK:  	ct.csc	ca1, 0(ca0)
  %x.coerce1.fca.0.extract = extractvalue %struct.InnerPtr %x.coerce1, 0
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %add = add i32 %call, %new_int
  %rem = urem i32 %add, 5

  ;; CHECK:  	auicgp	cs0, %cheriot_compartment_hi(dummies)
  ;; CHECK:  	cincoffset	cs0, cs0, %cheriot_compartment_lo_i(.LBB14_2)
  ;; CHECK:  	ct.csetbounds	cs0, cs0, %cheriot_compartment_size(dummies)
  ;; CHECK:  	ct.cincoffset	ca0, cs0, a0
  ;; CHECK:  	ct.csc	ca0, 8(csp)                     # 8-byte Folded Spill
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK:  	ct.clc	ca0, 32(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	ca1, 16(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.csc	ca0, 0(ca1)
  store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  ;; CHECK:  	ct.clw	a1, 28(csp)                     # 4-byte Folded Reload
  %call3 = tail call chericcallcce i32 @_Z8GetValuev()
  %add4 = add i32 %call3, %new_int
  %rem5 = urem i32 %add4, 5

  ;; CHECK:  	ct.cincoffset	ca1, cs0, a0
  %add.ptr6 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem5

  ;; CHECK:  	ct.clc	ca0, 8(csp)                     # 8-byte Folded Reload
  %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr6, 1, 0

  ;; CHECK:  	ct.cret
  ret %struct.ParentPtr %.fca.1.0.insert
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p200(ptr addrspace(200)) addrspace(200) #6

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p200(ptr addrspace(200)) addrspace(200) #6

; Function Attrs: nofree noinline norecurse nounwind
define dso_local chericcallcce %struct.TwoPointers @_Z8ChgPtrs3i11TwoPointersz(i32 noundef %n, ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1, ...) local_unnamed_addr addrspace(200) #7 {

;; CHECK:  _Z8ChgPtrs3i11TwoPointersz:             # @_Z8ChgPtrs3i11TwoPointersz
entry:
  %args = alloca ptr addrspace(200), align 8, addrspace(200)
  %_ = alloca i32, align 4, addrspace(200)
  call void @llvm.lifetime.start.p200(i64 8, ptr addrspace(200) nonnull %args) #8
  call void @llvm.va_start.p200(ptr addrspace(200) nonnull %args)
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  call void @llvm.lifetime.start.p200(i64 4, ptr addrspace(200) nonnull %_)
  %call = call chericcallcce i32 @_Z8GetValuev()
  store volatile i32 %call, ptr addrspace(200) %_, align 4, !tbaa !7
  store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11
  %cmp10 = icmp sgt i32 %n, 0
  br i1 %cmp10, label %for.body, label %for.cond.cleanup

for.cond.for.cond.cleanup_crit_edge:              ; preds = %for.body
  ;; CHECK:  auicgp	ca2, %cheriot_compartment_hi(dummies)
  ;; CHECK:  cincoffset	ca2, ca2, %cheriot_compartment_lo_i(.LBB15_5)
  ;; CHECK:  ct.csetbounds	ca2, ca2, %cheriot_compartment_size(dummies)

  ;; CHECK:  ct.cincoffset	ca3, ca2, s1
  %add.le = add i32 %call1, %0
  %rem.le = urem i32 %add.le, 5
  %add.ptr.le = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem.le

  ;; CHECK:  ct.cincoffset	ca1, ca2, a0
  %add4.le = add i32 %call3, %0
  %rem5.le = urem i32 %add4.le, 5
  %add.ptr6.le = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem5.le

  br label %for.cond.cleanup

for.body:                                         ; preds = %entry, %for.body
  %i.011 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %argp.cur = load ptr addrspace(200), ptr addrspace(200) %args, align 8
  %argp.next = getelementptr inbounds nuw i8, ptr addrspace(200) %argp.cur, i32 4
  store ptr addrspace(200) %argp.next, ptr addrspace(200) %args, align 8
  %0 = load i32, ptr addrspace(200) %argp.cur, align 4, !tbaa !7
  %call1 = call chericcallcce i32 @_Z8GetValuev()
  %call3 = call chericcallcce i32 @_Z8GetValuev()
  %inc = add nuw nsw i32 %i.011, 1
  %exitcond.not = icmp eq i32 %inc, %n
  br i1 %exitcond.not, label %for.cond.for.cond.cleanup_crit_edge, label %for.body, !llvm.loop !17

for.cond.cleanup:                                 ; preds = %for.cond.for.cond.cleanup_crit_edge, %entry
  %x.sroa.0.0.lcssa = phi ptr addrspace(200) [ %add.ptr.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce0, %entry ]
  %x.sroa.4.0.lcssa = phi ptr addrspace(200) [ %add.ptr6.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce1, %entry ]
  call void @llvm.va_end.p200(ptr addrspace(200) %args)
  call void @llvm.lifetime.end.p200(i64 4, ptr addrspace(200) nonnull %_)
  call void @llvm.lifetime.end.p200(i64 8, ptr addrspace(200) nonnull %args) #8

  ;; CHECK:  	ct.cmove	ca0, ca3
  %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %x.sroa.0.0.lcssa, 0
  %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %x.sroa.4.0.lcssa, 1

  ;; CHECK:  	ct.cret
  ret %struct.TwoPointers %.fca.1.insert

}

; Function Attrs: nofree noinline norecurse nounwind
define dso_local chericcallcce %struct.ParentPtr @_Z13ChgParentPtr3i9ParentPtrz(i32 noundef %n, ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1, ...) local_unnamed_addr addrspace(200) #7 {

;; CHECK:  _Z13ChgParentPtr3i9ParentPtrz:          # @_Z13ChgParentPtr3i9ParentPtrz
entry:
  %args = alloca ptr addrspace(200), align 8, addrspace(200)
  %_ = alloca i32, align 4, addrspace(200)
  %x.coerce1.fca.0.extract = extractvalue %struct.InnerPtr %x.coerce1, 0
  call void @llvm.lifetime.start.p200(i64 8, ptr addrspace(200) nonnull %args) #8
  call void @llvm.va_start.p200(ptr addrspace(200) nonnull %args)
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11
  call void @llvm.lifetime.start.p200(i64 4, ptr addrspace(200) nonnull %_)
  %call = call chericcallcce i32 @_Z8GetValuev()
  store volatile i32 %call, ptr addrspace(200) %_, align 4, !tbaa !7
  store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11
  %cmp12 = icmp sgt i32 %n, 0
  br i1 %cmp12, label %for.body, label %for.cond.cleanup

for.cond.for.cond.cleanup_crit_edge:              ; preds = %for.body
  %add.le = add i32 %call2, %0
  %rem.le = urem i32 %add.le, 5
  %add.ptr.le = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem.le
  %add5.le = add i32 %call4, %0
  %rem6.le = urem i32 %add5.le, 5
  %add.ptr7.le = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem6.le
  ;; CHECK:  	auicgp	ca2, %cheriot_compartment_hi(dummies)
  ;; CHECK:  	cincoffset	ca2, ca2, %cheriot_compartment_lo_i(.LBB16_6)
  ;; CHECK:  	ct.csetbounds	ca2, ca2, %cheriot_compartment_size(dummies)
  ;; CHECK:  	ct.cincoffset	ca0, ca2, s1
  ;; CHECK:  	ct.cincoffset	ca1, ca2, a1
  ;; CHECK:  	j	.LBB16_4
  br label %for.cond.cleanup

for.body:                                         ; preds = %entry, %for.body
  %i.013 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %argp.cur = load ptr addrspace(200), ptr addrspace(200) %args, align 8
  %argp.next = getelementptr inbounds nuw i8, ptr addrspace(200) %argp.cur, i32 4
  store ptr addrspace(200) %argp.next, ptr addrspace(200) %args, align 8
  %0 = load i32, ptr addrspace(200) %argp.cur, align 4, !tbaa !7
  %call2 = call chericcallcce i32 @_Z8GetValuev()
  %call4 = call chericcallcce i32 @_Z8GetValuev()
  %inc = add nuw nsw i32 %i.013, 1
  %exitcond.not = icmp eq i32 %inc, %n
  br i1 %exitcond.not, label %for.cond.for.cond.cleanup_crit_edge, label %for.body, !llvm.loop !18

for.cond.cleanup:                                 ; preds = %for.cond.for.cond.cleanup_crit_edge, %entry
  %x.sroa.0.0.lcssa = phi ptr addrspace(200) [ %add.ptr.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce0, %entry ]
  %x.sroa.4.0.lcssa = phi ptr addrspace(200) [ %add.ptr7.le, %for.cond.for.cond.cleanup_crit_edge ], [ %x.coerce1.fca.0.extract, %entry ]
  call void @llvm.va_end.p200(ptr addrspace(200) %args)
  call void @llvm.lifetime.end.p200(i64 4, ptr addrspace(200) nonnull %_)
  call void @llvm.lifetime.end.p200(i64 8, ptr addrspace(200) nonnull %args) #8
  %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %x.sroa.0.0.lcssa, 0
  %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %x.sroa.4.0.lcssa, 1, 0

  ;; CHECK: .LBB16_4:                               # %for.cond.cleanup
  ;; CHECK: 	ct.clc	cra, 56(csp)                    # 8-byte Folded Reload
  ;; CHECK: 	ct.clc	cs0, 48(csp)                    # 8-byte Folded Reload
  ;; CHECK: 	ct.clc	cs1, 40(csp)                    # 8-byte Folded Reload
  ;; CHECK: 	ct.cincoffset	csp, csp, 64
  ;; CHECK: 	ct.cret
  ret %struct.ParentPtr %.fca.1.0.insert

}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.TwoPointers @_Z8ChgPtrs4iiiii11TwoPointers(i32 noundef %n0, i32 noundef %n1, i32 noundef %n2, i32 noundef %n3, i32 noundef %n4, ptr addrspace(200) %x.coerce0, ptr addrspace(200) %x.coerce1) local_unnamed_addr addrspace(200) #1 {
;; CHECK: _Z8ChgPtrs4iiiii11TwoPointers:          # @_Z8ChgPtrs4iiiii11TwoPointers
;; CHECK:     mv	s1, a1
;; CHECK:     mv	s0, a0
;; CHECK:     ct.clc	ca0, 0(ct0)
;; CHECK:     ct.csc	ca0, 32(csp)                    # 8-byte Folded Spill
entry:
  ;; CHECK:  auicgp	ca0, %cheriot_compartment_hi(force_use)
  ;; CHECK:  	cincoffset	ca0, ca0, %cheriot_compartment_lo_i(.LBB17_2)
  ;; CHECK:  	ct.csc	ca0, 24(csp)                    # 8-byte Folded Spill
  ;; CHECK:  	ct.csc	ca5, 0(ca0)
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK: ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()

  %add = add i32 %n1, %n0
  %add1 = add i32 %add, %n2
  %add2 = add i32 %add1, %n3
  %add3 = add i32 %add2, %n4
  %add4 = add i32 %add3, %call
  %rem = urem i32 %add4, 5

  ;; CHECK: auicgp	ca3, %cheriot_compartment_hi(dummies)
  ;; CHECK: cincoffset	ca3, ca3, %cheriot_compartment_lo_i(.LBB17_3)
  ;; CHECK: ct.csc	ca3, 8(csp)                     # 8-byte Folded Spill
  ;; CHECK: ct.cincoffset	ca0, ca3, a0
  ;; CHECK: ct.csc	ca0, 16(csp)                    # 8-byte Folded Spill
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK: ct.clc	ca0, 32(csp)                    # 8-byte Folded Reload
  ;; CHECK: ct.clc	ca1, 24(csp)                    # 8-byte Folded Reload
  store ptr addrspace(200) %x.coerce1, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK: ct.ccall	_Z8GetValuev
  %call6 = tail call chericcallcce i32 @_Z8GetValuev()
  %add7 = add i32 %n1, %n0
  %add8 = add i32 %add7, %n2
  %add9 = add i32 %add8, %n3
  %add10 = add i32 %add9, %n4
  %add11 = add i32 %add10, %call6
  %rem12 = urem i32 %add11, 5

  ;; CHECK: ct.clc	ca1, 8(csp)                     # 8-byte Folded Reload
  ;; CHECK: ct.cincoffset	ca1, ca1, a0
  %add.ptr13 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem12

  ;; CHECK: ct.clc	ca0, 16(csp)                    # 8-byte Folded Reload
  %.fca.0.insert = insertvalue %struct.TwoPointers poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.insert = insertvalue %struct.TwoPointers %.fca.0.insert, ptr addrspace(200) %add.ptr13, 1

  ;; CHECK: ct.cret
  ret %struct.TwoPointers %.fca.1.insert
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none)
define dso_local chericcallcce %struct.ParentPtr @_Z13ChgParentPtr4iiiii9ParentPtr(i32 noundef %n0, i32 noundef %n1, i32 noundef %n2, i32 noundef %n3, i32 noundef %n4, ptr addrspace(200) %x.coerce0, %struct.InnerPtr %x.coerce1) local_unnamed_addr addrspace(200) #1 {

;; CHECK:  _Z13ChgParentPtr4iiiii9ParentPtr:       # @_Z13ChgParentPtr4iiiii9ParentPtr
;; CHECK:  	mv	s1, a1
;; CHECK:  	mv	s0, a0
;; CHECK:  	ct.clc	ca0, 0(ct0)
;; CHECK:  	ct.csc	ca0, 32(csp)                    # 8-byte Folded Spill
entry:

  ;; CHECK:  	auicgp	ca0, %cheriot_compartment_hi(force_use)
  ;; CHECK:  	cincoffset	ca0, ca0, %cheriot_compartment_lo_i(.LBB18_2)
  ;; CHECK:  	ct.csc	ca0, 24(csp)                    # 8-byte Folded Spill
  ;; CHECK:  	ct.csc	ca5, 0(ca0)
  %x.coerce1.fca.0.extract = extractvalue %struct.InnerPtr %x.coerce1, 0
  store ptr addrspace(200) %x.coerce0, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call = tail call chericcallcce i32 @_Z8GetValuev()
  %add = add i32 %n1, %n0
  %add2 = add i32 %add, %n2
  %add3 = add i32 %add2, %n3
  %add4 = add i32 %add3, %n4
  %add5 = add i32 %add4, %call
  %rem = urem i32 %add5, 5

  ;; CHECK:  	auicgp	ca3, %cheriot_compartment_hi(dummies)
  ;; CHECK:  	cincoffset	ca3, ca3, %cheriot_compartment_lo_i(.LBB18_3)
  ;; CHECK:  	ct.csetbounds	ca3, ca3, %cheriot_compartment_size(dummies)
  ;; CHECK:  	ct.csc	ca3, 8(csp)                     # 8-byte Folded Spill
  ;; CHECK:  	ct.cincoffset	ca0, ca3, a0
  ;; CHECK:  	ct.csc	ca0, 16(csp)                    # 8-byte Folded Spill
  %add.ptr = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem

  ;; CHECK:  	ct.clc	ca0, 32(csp)                    # 8-byte Folded Reload
  ;; CHECK:  	ct.clc	ca1, 24(csp)                    # 8-byte Folded Reload
  store ptr addrspace(200) %x.coerce1.fca.0.extract, ptr addrspace(200) @force_use, align 8, !tbaa !11

  ;; CHECK:  	ct.ccall	_Z8GetValuev
  %call7 = tail call chericcallcce i32 @_Z8GetValuev()
  %add8 = add i32 %n1, %n0
  %add9 = add i32 %add8, %n2
  %add10 = add i32 %add9, %n3
  %add11 = add i32 %add10, %n4
  %add12 = add i32 %add11, %call7
  %rem13 = urem i32 %add12, 5

  ;; CHECK:  	ct.cincoffset	ca1, ca1, a0
  %add.ptr14 = getelementptr inbounds nuw i32, ptr addrspace(200) @dummies, i32 %rem13

  ;; CHECK:  	ct.clc	ca0, 16(csp)                    # 8-byte Folded Reload
  %.fca.0.insert = insertvalue %struct.ParentPtr poison, ptr addrspace(200) %add.ptr, 0
  %.fca.1.0.insert = insertvalue %struct.ParentPtr %.fca.0.insert, ptr addrspace(200) %add.ptr14, 1, 0

  ;; CHECK:  	ct.cret
  ret %struct.ParentPtr %.fca.1.0.insert
}

attributes #0 = { mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, argmem: none, inaccessiblemem: none) "cheri-compartment"="example" "interrupt-state"="enabled" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+xcheri" }
attributes #1 = { mustprogress nofree noinline norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none) "cheri-compartment"="example" "interrupt-state"="enabled" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+xcheri" }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(readwrite, inaccessiblemem: none) "cheri-compartment"="example" "interrupt-state"="enabled" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+xcheri" }
attributes #3 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #4 = { mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) "cheri-compartment"="example" "interrupt-state"="enabled" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+xcheri" }
attributes #5 = { nofree noinline norecurse nosync nounwind "cheri-compartment"="example" "interrupt-state"="enabled" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+xcheri" }
attributes #6 = { mustprogress nocallback nofree nosync nounwind willreturn }
attributes #7 = { nofree noinline norecurse nounwind "cheri-compartment"="example" "interrupt-state"="enabled" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+32bit,+c,+xcheripurecap,+e,+m,+xcheri" }
attributes #8 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !4, !5}
!llvm.ident = !{!6}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, !"target-abi", !"cheriot"}
!2 = !{i32 6, !"riscv-isa", !3}
!3 = !{!"rv32e2p0_m2p0_c2p0_zmmul1p0_xcheri0p0"}
!4 = !{i32 1, !"Code Model", i32 1}
!5 = !{i32 8, !"SmallDataLimit", i32 0}
!6 = !{!"clang version 20.1.3"}
!7 = !{!8, !8, i64 0}
!8 = !{!"int", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}
!11 = !{!12, !12, i64 0}
!12 = !{!"p1 int", !13, i64 0}
!13 = !{!"any pointer", !9, i64 0}
!14 = distinct !{!14, !15, !16}
!15 = !{!"llvm.loop.mustprogress"}
!16 = !{!"llvm.loop.unroll.disable"}
!17 = distinct !{!17, !15, !16}
!18 = distinct !{!18, !15, !16}
