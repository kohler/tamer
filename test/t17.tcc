// -*- mode: c++ -*-
/* Copyright (c) 2013, Eddie Kohler
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

tamed void readit(tamer::fd fd) {
    tvars { char buf[1024]; size_t x; int ret; }
    twait [std::string("READING SOMETHING", 10) + " OR OTHER"] {
        fd.read(buf, sizeof(buf), x, make_event(ret));
    }
    std::cout << ret << " returned, " << x << " bytes read\n";
}

tamed void timeit() {
    twait ["HELLO"] { tamer::at_delay(0.5, make_event()); }
}

tamed void asapit() {
    twait { tamer::at_asap(make_event()); }
}

tamed void printit(tamer::fd& wfd) {
    twait { tamer::at_delay(0.1, make_event()); }
    asapit();
    {
        std::vector<std::string> x;
        tamer::driver::main->blocked_locations(x);
        for (size_t i = 0; i != x.size(); ++i)
            std::cout << x[i] << std::endl;
    }
    wfd.close();
}

int main(int, char**) {
    tamer::initialize();
    tamer::fd fr, fw;
    tamer::fd::pipe(fr, fw);
    readit(fr);
    timeit();
    asapit();
    printit(fw);
    tamer::loop();
    tamer::cleanup();
}
