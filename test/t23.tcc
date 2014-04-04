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

tamed void f(double x, tamer::event<const char*> done) {
    twait { tamer::at_delay(x, make_event()); }
    done(x > 0.1 ? "slower" : "faster");
}

tamed void g(double x) {
    tamed { const char* str; }
    twait { f(x, make_event(str)); }
    printf("%g: %s\n", x, str);
}

int main(int, char**) {
    tamer::initialize();
    tamer::set_time_type(tamer::time_virtual);
    g(0.15);
    g(0.05);
    tamer::loop();
    tamer::cleanup();
    printf("OK!\n");
}
