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

class suicider : public tamer::tamed_class {
  public:
    suicider(int who, double destroy_after)
        : who_(who) {
        destroyer(destroy_after);
    }
  private:
    int who_;
    tamed void destroyer(double after);
};

tamed void suicider::destroyer(double after) {
    twait { tamer::at_delay(after, make_event()); }
    printf("suicider %d dying\n", who_);
    delete this;
}

tamed void f() {
    tamed { tamer::destroy_guard g(new suicider(1, 0.2)); }
    printf("f @0\n");
    twait { tamer::at_delay(0.11, make_event()); }
    printf("f @1\n");
    twait { tamer::at_delay(0.11, make_event()); }
    printf("f @2\n");
}

tamed void g() {
    tamed { tamer::destroy_guard guard0(new suicider(2, 0.4)),
            guard1(new suicider(3, 0.14)); }
    printf("g @0\n");
    twait { tamer::at_delay(0.11, make_event()); }
    printf("g @1\n");
    twait { tamer::at_delay(0.11, make_event()); }
    printf("g @2\n");
    twait { tamer::at_delay(0.22, make_event()); }
    printf("g @3\n");
}

int main(int, char**) {
    tamer::initialize();
    tamer::set_time_type(tamer::time_virtual);
    f();
    g();
    tamer::loop();
    tamer::cleanup();
    printf("OK!\n");
}
