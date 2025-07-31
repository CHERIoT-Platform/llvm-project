// RUN: %cheri_purecap_cc1 -analyze %s \
// RUN:   -analyzer-checker=core,cheri.SubObjectRepresentability

struct a {
  unsigned b;
};
void c(unsigned a::*d) {
  d == &a::b;
  c(&a::b);
}
