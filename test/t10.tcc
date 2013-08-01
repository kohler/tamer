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
size_t npass = 0;

tamed void passoff(tamer::fd& r, tamer::fd& w) {
    tvars { char c; size_t x; int ret; }
    while (1) {
        twait { r.read(&c, 1, x, make_event(ret)); }
        if (x) {
            ++npass;
            twait { w.write(&c, 1, x, make_event(ret)); }
        }
    }
}

tamed void writeit(tamer::fd& w) {
    tvars { char c = 'A'; size_t x; int ret; }
    twait { w.write(&c, 1, x, make_event(ret)); }
}

tamed void readandexit(tamer::fd& r) {
    tvars { char c; size_t x; int ret; }
    twait { r.read(&c, 1, x, make_event(ret)); }
    std::cerr << "Got character " << c << " after " << npass << " passes\n";
    exit(1);
}

int main(int, char**) {
    tamer::initialize();
    tamer::fd* fds = new tamer::fd[1000];
    for (int i = 0; i < 1000; i += 2)
        tamer::fd::pipe(fds[i], fds[i+1]);
    for (int i = 0; i < 998; i += 2)
        passoff(fds[i], fds[i+3]);
    writeit(fds[1]);
    readandexit(fds[998]);
    tamer::loop();
    tamer::cleanup();
}
