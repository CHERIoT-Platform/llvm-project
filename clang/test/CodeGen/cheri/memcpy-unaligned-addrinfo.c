// RUN: %riscv64_cheri_purecap_cc1 %s -S -O2 -o /dev/null -verify -debug-info-kind=standalone
// REQUIRES: riscv-registered-target

// Partial port of the MIPS test test/CodeGen/Mips/cheri/memcpy-unaligned-addrinfo.c
// (issue #301). It preserves the backend "found underaligned load/store of capability type"
// warning (-Wcheri-inefficient), emitted from the target-independent
// SelectionDAG legalizer when the whole-capability copy that SROA collapses to
// a single underaligned capability load/store is expanded to a tag-preserving
// memcpy() call. The SROA bug itself is covered by
// CHERI-Generic/Inputs/memcpy-unaligned-addrinfo.ll, and the
// frontend-memtransfer-type attribute by clang/test/CodeGen/cheri/memcpy-unaligned.c.

struct addrinfo {
  char *b;
};

struct addrinfo c(char *a) {
  struct addrinfo d;
  __builtin_memcpy(&d, a, sizeof(struct addrinfo));
  // expected-warning@-1{{found underaligned load of capability type (aligned to 1 bytes instead of 16). Will use memcpy() instead of capability load to preserve tags if it is aligned correctly at runtime}}
  // expected-note@-2{{use __builtin_assume_aligned() or cast}}
  return d;
}

struct group {
  char *b;
};
void do_stuff(struct group *g);

void copy_group2(const char *a, char *buffer) {
  // derived from the unaligned memcpy used in getgrent
  // Note: this will result in an unaligned memcpy
  __builtin_memcpy(buffer, &a, sizeof(char *));
  // expected-warning@-1{{found underaligned store of capability type (aligned to 1 bytes instead of 16). Will use memcpy() instead of capability load to preserve tags if it is aligned correctly at runtime}}
  // expected-note@-2{{use __builtin_assume_aligned()}}
  struct group *g = (struct group *)buffer;
  do_stuff(g);
}
