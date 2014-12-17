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

int main(int, char**) {
    tamer::initialize();
    tamer::set_time_type(tamer::time_virtual);

    tamer::event<> e[20];
    tamer::rendezvous<> r;
    tamer::at_delay_msec(11, e[0] = r.make_event());
    tamer::at_delay_msec(12, e[1] = r.make_event());
    tamer::at_delay_msec(13, e[2] = r.make_event());
    tamer::at_delay_msec(20, e[3] = r.make_event());
    tamer::at_delay_msec(17, e[4] = r.make_event());
    tamer::at_delay_msec(18, e[5] = r.make_event());
    tamer::at_delay_msec(15, e[6] = r.make_event());
    tamer::at_delay_msec(14, e[7] = r.make_event());
    tamer::at_delay_msec(16, e[8] = r.make_event());
    tamer::at_delay_msec(21, e[9] = r.make_event());
    tamer::at_delay_msec(19, e[10] = r.make_event());

    e[1]();
    e[2]();
    e[4]();
    e[5]();
    e[7]();

    tamer::at_delay_msec(4, e[11] = r.make_event());

    tamer::once();

    printf("%06ld\n", (long) tamer::recent().tv_usec);
    assert(tamer::recent().tv_usec == 4002);

    tamer::loop();
    tamer::cleanup();
    printf("OK!\n");
}
