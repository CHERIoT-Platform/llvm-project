; RUN: llc @HYBRID_HARDFLOAT_ARGS@ -O2 -verify-machineinstrs %s -o - | FileCheck %s

@array = common global [2 x ptr addrspace(200)] zeroinitializer, align @CAP_BYTES@

;; Check that we convert the index to an offset by multiplying by the
;; capability size:
define ptr addrspace(200) @get(i32 signext %x) {
entry:
  %idxprom = sext i32 %x to i64
  %arrayidx = getelementptr inbounds [2 x ptr addrspace(200)], ptr @array, i64 0, i64 %idxprom
  %0 = load ptr addrspace(200), ptr %arrayidx, align @CAP_BYTES@
  ret ptr addrspace(200) %0
}
