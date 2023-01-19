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
#include <string.h>
#include <tamer/tamer.hh>
#include <tamer/lock.hh>

// Expected output:
// A   (60ms pass)
// BX  (40ms pass)
// A'
// B   (250ms pass)
// E
// F

static tamer::mutex m;
static int x = 0;

tamed void f1() {
    twait { m.acquire(make_event()); }
    printf("A\n");
    assert(++x == 1);
    twait { tamer::at_delay_msec(100, make_event()); }
    printf("A'\n");
    assert(++x == 3);
    m.release();
}

tamed void f2() {
    twait { tamer::at_delay_msec(50, make_event()); }
    twait { m.acquire_shared(make_event()); }
    printf("B\n");
    assert(++x == 4);
    twait {
        tamer::at_delay_msec(250, make_event());
    }
    m.release_shared();
    printf("E\n");
    assert(++x == 5);
}

tamed void f3() {
    tvars { int outcome; }
    twait { tamer::at_delay_msec(50, make_event()); }
    twait { m.acquire_shared(tamer::with_timeout_msec(10, make_event(), outcome)); }
    if (outcome < 0) {
        printf("BX\n");
        assert(++x == 2);
    } else {
        printf("B'\n");
        x += 10;
        twait { tamer::at_delay_msec(10, make_event()); }
        printf("C\n");
        x += 10;
        m.release_shared();
        printf("D\n");
        x += 10;
        assert(0);
    }
}

tamed void f4() {
    tvars { int outcome; }
    twait { tamer::at_delay_msec(100, make_event()); }
    twait { m.acquire(tamer::with_timeout_msec(1000, make_event(), outcome)); }
    //twait { m.acquire(make_event()); }
    if (outcome < 0) {
        printf("FX %d %s\n", outcome, strerror(-outcome));
        x += 20;
        assert(0);
    } else {
        printf("F\n");
        assert(++x == 6);
    }
    m.release();
}

int main(int, char **) {
    tamer::initialize();
    f1();
    f2();
    f3();
    f4();
    tamer::loop();
    tamer::cleanup();
}
