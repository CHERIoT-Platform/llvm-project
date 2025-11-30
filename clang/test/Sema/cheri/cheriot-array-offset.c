// RUN: %riscv32_cheri_cc1 "-triple" "riscv32cheriot-unknown-unknown" "-target-abi" "cheriot" -verify %s 

void bar(void*);

void foo(int i) {
    int buf[4];
    bar(buf + i + 1);
    bar(buf + i + 4); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf + i + 5); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf + i - 1);
    bar(buf + i - 4); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf + i - 5); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - i + 1);
    bar(buf - i + 4); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - i + 5); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - i - 1);
    bar(buf - i - 4); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - i - 5); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}

    bar(buf + 1 + i);
    bar(buf + 4 + i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf + 5 + i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - 1 + i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - 4 + i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - 5 + i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf + 1 - i);
    bar(buf + 4 - i);
    bar(buf + 5 - i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - 1 - i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - 4 - i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}
    bar(buf - 5 - i); // expected-warning{{offset pattern of 'buf' may create an invalid intermediate capability; consider reassociating the offsets together}}

    bar(buf + 0 + 1);
    bar(buf + 0 + 4);
    bar(buf + 0 + 5);
    bar(buf + 0 - 1);
    bar(buf + 0 - 4);
    bar(buf + 0 - 5);
    bar(buf - 0 + 1);
    bar(buf - 0 + 4);
    bar(buf - 0 + 5);
    bar(buf - 0 - 1);
    bar(buf - 0 - 4);
    bar(buf - 0 - 5);
}
