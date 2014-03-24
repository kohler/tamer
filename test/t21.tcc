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
#include <tamer/tamer.hh>
#include <tamer/channel.hh>
using tamer::event;
using tamer::channel;

tamed void take(channel<int>& c) {
    tamed { int i, x; }
    for (i = 0; i != 4; ++i) {
        twait { c.pop_front(make_event(x)); }
        std::cout << i << " " << x << "\n";
    }
}

tamed void put(channel<int>& c) {
    tamed { int i; }
    for (i = 0; i != 3; ++i) {
        c.push_back(i);
        std::cout << "block\n";
        twait { tamer::at_delay(0.01, make_event()); }
    }
}

int main(int, char**) {
    channel<int> c;
    tamer::initialize();
    c.push_back(-1);
    take(c);
    put(c);
    tamer::loop();
    tamer::cleanup();
    std::cout << "OK!\n";
}
