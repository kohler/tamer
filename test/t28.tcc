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
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <tamer/tamer.hh>

struct A {
    tamed template <typename F>
    void doit(F func, tamer::event<> done);

    tamed void helper(int foo, tamer::event<> done);
};

struct B {
    tamed void passit(A* a, tamer::event<> done);
};


tamed template <typename F>
void A::doit(F func, tamer::event<> done) {
    twait { helper(func(27), make_event()); }
    done();
}

tamed void A::helper(int foo, tamer::event<> done) {
    std::cout << "in helper " << foo << std::endl;
    done();
}

#if TAMER_HAVE_CXX_LAMBDAS
tamed void B::passit(A* a, tamer::event<> done) {
    twait { a->doit([=](int b){ return b * 2; }, make_event()); }
    done();
}
#else
tamed void B::passit(A* a, tamer::event<> done) {
    printf("no c++11 lambda support\n");
    done();
}
#endif


tamed void function() {
    tvars {
        A a;
        B b;
    }

    twait { b.passit(&a, make_event()); }
}

int main(int, char**) {
    tamer::initialize();
    function();
    tamer::loop();
    tamer::cleanup();
}
