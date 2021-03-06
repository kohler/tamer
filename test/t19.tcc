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
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <tamer/fd.hh>
using tamer::event;

tamed void run() {
    tamed { std::vector<int> v; }
    tamer::at_delay(0.1, tamer::bind(tamer::push_back_event(v), 10039));
    assert(v.empty());
    twait { tamer::at_delay(0.05, make_event()); }
    assert(v.empty());
    twait { tamer::at_delay(0.06, make_event()); }
    assert(v.size() == 1);
    assert(v[0] == 10039);
}

tamed void run2() {
    tamed { int x[4]; int* it = x; }
    tamer::at_delay(0.02, tamer::bind(tamer::output_event(it), 2));
    tamer::at_delay(0.03, tamer::bind(tamer::output_event(it), 3));
    tamer::at_delay(0.01, tamer::bind(tamer::output_event(it), 1));
    assert(it == x);
    twait { tamer::at_delay(0.05, make_event()); }
    assert(it == x + 3 && x[0] == 1 && x[1] == 2 && x[2] == 3);
}

int main(int, char**) {
    tamer::initialize();
    run();
    run2();
    tamer::loop();
    tamer::cleanup();
    std::cout << "OK!\n";
}
