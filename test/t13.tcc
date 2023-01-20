// -*- mode: c++ -*-
/* Copyright (c) 2007, Eddie Kohler
 * Copyright (c) 2007, Regents of the University of California
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
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <tamer/tamer.hh>
#include <tamer/lock.hh>
using namespace tamer;

tamer::mutex m;

tamed void exclusive(int which) {
    tvars { int ret = 0; }
    if (which == 4 && 0)
        twait { m.acquire(with_timeout_msec(120, make_event(), ret)); }
    else
        twait { m.acquire(make_event()); }
    if (ret >= 0) {
        printf("%d: acquired\n", which);
        twait { tamer::at_delay_msec(100, make_event()); }
        m.release();
        printf("%d: released\n", which);
    } else
        printf("%d: canceled\n", which);
}

tamed void shared(int which) {
    twait { m.acquire_shared(make_event()); }
    printf("%d: acquired shared\n", which);
    twait { tamer::at_delay_msec(100, make_event()); }
    m.release_shared();
    printf("%d: released shared\n", which);
}

int main(int, char *[]) {
    tamer::initialize();
    exclusive(1);
    shared(2);
    shared(3);
    exclusive(4);
    shared(5);
    tamer::loop();
    tamer::cleanup();
}
