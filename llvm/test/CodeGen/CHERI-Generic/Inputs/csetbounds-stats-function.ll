; !DO NOT AUTOGEN!
;; Check that the ELF symbol-table feeder in `ELFObjectWriter.cpp`
;; records csetbounds entries for each defined function. This is a
;; target-independent feeder, so the CSV has one entry per defined
;; function on every CHERI backend (the exact size value depends on
;; the per-arch instruction encoding; matched by regex).
; RUN: rm -f %t.csv
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -O0 -o /dev/null -filetype=obj \
; RUN:    -collect-csetbounds-output=%t.csv -collect-csetbounds-stats=csv
; RUN: FileCheck %s -input-file=%t.csv -check-prefix CSV

define internal i32 @maybe_inline(i32 %arg) unnamed_addr {
  entry:
    %result = add i32 %arg, 5
    ret i32 %result
}

define i32 @test_func() {
entry:
  %call = call i32 @external_fn()
  %call2 = call i32 @maybe_inline(i32 4)
  %result = add i32 %call, %call2
  ret i32 %result
}

declare i32 @external_fn()

; CSV: alignment_bits,size,kind,source_loc,compiler_pass,details
; CSV-NEXT: 2,{{[0-9]+}},c,"UNKNOWN","ELF symbol table","Function maybe_inline"
; CSV-NEXT: 2,{{[0-9]+}},c,"UNKNOWN","ELF symbol table","Function test_func"
; CSV-EMPTY:
