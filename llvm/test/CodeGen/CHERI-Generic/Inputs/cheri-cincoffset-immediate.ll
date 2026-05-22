; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=PURECAP

define ptr addrspace(200) @imm(ptr addrspace(200) readnone %a) local_unnamed_addr nounwind {
entry:
  %add.ptr = getelementptr inbounds i8, ptr addrspace(200) %a, i64 1023
  ret ptr addrspace(200) %add.ptr
}

define ptr addrspace(200) @imm1(ptr addrspace(200) readnone %a) local_unnamed_addr nounwind {
entry:
  %add.ptr = getelementptr inbounds i8, ptr addrspace(200) %a, i64 -1024
  ret ptr addrspace(200) %add.ptr
}

define ptr addrspace(200) @reg(ptr addrspace(200) readnone %a) local_unnamed_addr nounwind {
entry:
  %add.ptr = getelementptr inbounds i8, ptr addrspace(200) %a, i64 1024
  ret ptr addrspace(200) %add.ptr
}

define ptr addrspace(200) @reg1(ptr addrspace(200) readnone %a) local_unnamed_addr nounwind {
entry:
  %add.ptr = getelementptr inbounds i8, ptr addrspace(200) %a, i64 -1025
  ret ptr addrspace(200) %add.ptr
}
