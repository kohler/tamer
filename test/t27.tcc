// -*- mode: c++ -*-
/* Copyright (c) 2014, Eddie Kohler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Tamer LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Tamer LICENSE file; the license in that file is
 * legally binding.
 */
#include "config.h"
#undef TAMER_DEBUG
#include <stdio.h>
#include <tamer/tamer.hh>

class foo : public tamer::tamed_class {
  public:
    static int nfoos;
    foo() { ++nfoos; }
    ~foo() { --nfoos; }
    tamed virtual void delay(double d);
};

class bar : public foo {
    tamed virtual void delay(double d);
};

int foo::nfoos;

tamed void foo::delay(double d) {
    tamed { foo fart; }
    twait { tamer::at_delay(d, make_event()); }
    printf("foo delay %g happened\n", d);
}

tamed void bar::delay(double d) {
    tamed { foo fart; }
    twait { tamer::at_delay(d, make_event()); }
    printf("bar delay %g happened\n", d);
}

tamed void f(foo* f0, foo* f1) {
    f0->delay(0.1);
    f1->delay(0.2);
    f0->delay(0.3);
    f1->delay(0.4);
    twait { tamer::at_delay(0.25, make_event()); }
    printf("return\n");
}

int main(int, char**) {
    foo f0;
    bar b0;
    tamer::initialize();
    tamer::set_time_type(tamer::time_virtual);
    f(&f0, &b0);
    tamer::loop();
    tamer::cleanup();
    printf("OK! %d foos\n", foo::nfoos);
}
