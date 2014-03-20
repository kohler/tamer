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
    tamed { std::string test[1024]; }
    for (int i = 0; i != 1024; ++i) {
        char buf[5];
        sprintf(buf, "%d", i);
        test[i] = std::string(buf);
    }
    twait { tamer::at_delay(0.01, make_event()); }
    std::cout << test[1023] << std::endl;
}

tamed void run2(std::string x[4]) {
    twait { tamer::at_delay(0.02, make_event()); }
    std::cout << x[3] << std::endl;
}

tamed void run3() {
    tamed { std::string test[1024][1024]; }
    for (int i = 0; i != 1024; ++i) {
        char buf[5];
        sprintf(buf, "%d", i);
        test[i][i] = std::string(buf);
    }
    twait { tamer::at_delay(0.03, make_event()); }
    std::cout << test[1023][1023] << std::endl;
}

tamed void run4(std::string x[3][2]) {
    twait { tamer::at_delay(0.04, make_event()); }
    std::cout << x[2][1] << std::endl;
}

int main(int, char**) {
    std::string x[4] = {"", "a", "b", "c"};
    std::string x2[3][2] = {{"aa", "bb"}, {"cc", "dd"}, {"ee", "ff"}};
    tamer::initialize();
    run();
    run2(x);
    run3();
    run4(x2);
    tamer::loop();
    tamer::cleanup();
    std::cout << "OK!\n";
}
