// RUN: %clang_analyze_cc1 -triple x86_64-unknown-linux -verify %s \
// RUN:   -analyzer-checker=core,optin.portability.PointerAlignment

// Trailing-zero tracking for '/' and '>>'. Dividing by a non-power-of-two, or
// shifting right by an unknown amount, gives no alignment information; both used
// to be treated as preserving it, hiding these misaligned casts.

typedef unsigned long uintptr_t;
__int128 a[64];

void div_odd(void) {
  uintptr_t x = (uintptr_t)a / 3;
  *(__int128 *)x = 0; // expected-warning{{Pointer value aligned to a 1 byte boundary cast to type '__int128 *' with 16-byte alignment}}
}

void div_pow2(void) {
  uintptr_t x = (uintptr_t)a / 2;
  *(__int128 *)x = 0; // expected-warning{{Pointer value aligned to a 8 byte boundary cast to type '__int128 *' with 16-byte alignment}}
}

void shr_unknown(unsigned s) {
  uintptr_t x = (uintptr_t)a >> s;
  *(__int128 *)x = 0; // expected-warning{{Pointer value aligned to a 1 byte boundary cast to type '__int128 *' with 16-byte alignment}}
}

void shr_const(void) {
  uintptr_t x = (uintptr_t)a >> 1;
  *(__int128 *)x = 0; // expected-warning{{Pointer value aligned to a 8 byte boundary cast to type '__int128 *' with 16-byte alignment}}
}
