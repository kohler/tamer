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
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
#include <tamer/channel.hh>
using tamer::event;
using tamer::channel;

std::ostream& operator<<(std::ostream& str, timeval x) {
    char buf[40];
    int len = sprintf(buf, "%ld.%06ld", (long) x.tv_sec, (long) x.tv_usec);
    str.write(buf, len);
    return str;
}

tamed void f(tamer::fd fd) {
    tamed { char buf[40]; size_t len; int r; }
    std::cout << tamer::recent() << "\n";
    twait { tamer::at_delay(100, make_event()); }
    std::cout << tamer::recent() << "\n";
    twait { fd.read(buf, 20, tamer::add_timeout(100, make_event(len, r))); }
    std::cout << len << " ";
    std::cout.write(buf, len);
    std::cout << "\n" << tamer::recent() << "\n";
}

int main(int, char**) {
    tamer::initialize();
    tamer::set_time_type(tamer::time_virtual);
    tamer::fd::make_nonblocking(0);
    f(tamer::fd(0));
    tamer::loop();
    tamer::cleanup();
    std::cout << "OK!\n";
}
