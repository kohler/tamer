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

int global_x = 19324;

tamed void f(tamer::event<int> done) {
    tvars {
        int& x = global_x;
    }
    done(x);
}

tamed void g() {
    tvars { int x; }
    twait { f(tamer::make_event(x)); }
    fprintf(stdout, "Got %d\n", x);
}

int main(int, char**) {
    tamer::initialize();
    g();
    tamer::loop();
    tamer::cleanup();
}
