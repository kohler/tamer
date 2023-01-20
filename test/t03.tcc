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
#include <tamer/fd.hh>

tamed void read_sucka(tamer::fd &f) {
    tvars {
        int ret(-1);
        char buf[40];
        size_t amt(0);
    }
    twait {
        f.read(buf, 40, amt, tamer::add_timeout(0.5, make_event(ret), -ETIMEDOUT));
    }
    if (ret < 0) {
        printf("got error %d (%s): %ld: %.*s\n", ret, strerror(-ret), (long) amt, (int) amt, buf);
    } else {
        printf("got %d: %ld: %.*s\n", ret, (long) amt, (int) amt, buf);
    }
}

tamed void time_sucka() {
    twait { tamer::at_delay_msec(250, make_event()); }
}

tamed void close_sucka(tamer::fd &f) {
    twait { tamer::at_delay_msec(125, make_event()); }
    f.close();
}


int main(int, char **) {
    tamer::initialize();
    tamer::fd::make_nonblocking(0);
    tamer::fd mystdin(0);
    read_sucka(mystdin);
    time_sucka();
    close_sucka(mystdin);
    tamer::loop();
    tamer::cleanup();
}
