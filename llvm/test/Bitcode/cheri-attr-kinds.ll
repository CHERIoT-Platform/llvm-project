; The CHERI attributes round-trip and are encoded with kinds 65536 and 65537,
; outside the range used by upstream attributes.

; RUN: llvm-as < %s | llvm-dis | FileCheck %s
; RUN: llvm-as < %s | llvm-bcanalyzer -dump | FileCheck %s --check-prefix=ENC

; CHECK: define void @must() #0
; CHECK: define void @nopreserve() #1
; CHECK: attributes #0 = { must_preserve_cheri_tags }
; CHECK: attributes #1 = { no_preserve_cheri_tags }

; ENC: <ENTRY op0=1 op1=4294967295 op2=0 op3=65536/>
; ENC: <ENTRY op0=2 op1=4294967295 op2=0 op3=65537/>

define void @must() #0 {
  ret void
}

define void @nopreserve() #1 {
  ret void
}

attributes #0 = { must_preserve_cheri_tags }
attributes #1 = { no_preserve_cheri_tags }
