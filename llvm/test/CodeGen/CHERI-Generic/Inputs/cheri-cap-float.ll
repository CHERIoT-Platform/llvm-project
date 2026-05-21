; RUN: llc @HYBRID_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=HYBRID
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=PURECAP

; Function Attrs: nounwind memory(read)
define double @load64(ptr addrspace(200) nocapture readonly %x) #0 {
entry:
  %0 = load double, ptr addrspace(200) %x, align 8
  ret double %0
}

; Function Attrs: nounwind memory(read)
define float @load32(ptr addrspace(200) nocapture readonly %x) #0 {
entry:
  %0 = load float, ptr addrspace(200) %x, align 4
  ret float %0
}

; Function Attrs: nounwind
define void @store64(ptr addrspace(200) nocapture %x, double %y) #1 {
entry:
  store double %y, ptr addrspace(200) %x, align 8
  ret void
}

; Function Attrs: nounwind
define void @store32(ptr addrspace(200) nocapture %x, float %y) #1 {
entry:
  store float %y, ptr addrspace(200) %x, align 4
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { nounwind "use-soft-float"="false" }
