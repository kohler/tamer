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
#include <stdio.h>
#include <assert.h>
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
#include <tamer/bufferedio.hh>
using namespace tamer;

tamed void child(std::vector<int> ary, tamer::event<> done) {
    tvars { std::vector<int> x(ary.size()); }
    assert(ary.size() == 3);
    assert(x.size() == 3);
    twait { tamer::at_delay(0.1, make_event()); }
    assert(ary.size() == 3);
    assert(x.size() == 3);
    assert(ary[0] == 69);
    assert(ary[1] == 70);
    assert(ary[2] == 260);
    done();
}

tamed void f() {
    tvars { std::vector<int> x; }
    x.push_back(69);
    x.push_back(70);
    x.push_back(260);
    twait {
        child(x, make_event());
        x.clear();
    }
    assert(x.size() == 0);
}

int main(int, char *[]) {
    tamer::initialize();
    f();
    tamer::loop();
    tamer::cleanup();
    printf("All OK\n");
}
