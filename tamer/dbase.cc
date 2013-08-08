/* Copyright (c) 2007-2013, Eddie Kohler
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
#include "config.h"
#include <tamer/tamer.hh>
#include <sys/select.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

namespace tamer {
namespace tamerpriv {
timeval now;
bool now_updated;
} // namespace tamerpriv

driver* driver::main;
driver* driver::indexed[capacity];
int driver::next_index;

driver::driver() {
    if (next_index < capacity) {
        index_ = next_index;
        ++next_index;
    } else
        for (index_ = 0; index_ != capacity && indexed[index_]; ++index_)
            /* do nothing */;
    assert(index_ < capacity);
    indexed[index_] = this;
}

driver::~driver() {
    indexed[index_] = 0;
    if (main == this)
        main = 0;
}

bool initialize(int flags) {
    if (driver::main)
        return true;

    if (!(flags & (use_tamer | use_libevent | use_libev))) {
        const char* dname = getenv("TAMER_DRIVER");
        if (dname && strcmp(dname, "libev") == 0)
            flags |= use_libev;
        else if (dname && strcmp(dname, "libevent") == 0)
            flags |= use_libevent;
        else
            flags |= use_tamer;
    }

    if (!driver::main && (flags & use_libev))
        driver::main = driver::make_libev();
    if (!driver::main && (flags & use_libevent))
        driver::main = driver::make_libevent();
    if (!driver::main && !(flags & use_tamer) && (flags & no_fallback))
        return false;
    if (!driver::main)
        driver::main = driver::make_tamer();

    if (!(flags & keep_sigpipe))
        signal(SIGPIPE, SIG_IGN);
    return true;
}

void cleanup() {
    delete driver::main;
    driver::main = 0;
}

void driver::at_delay(double delay, event<> e)
{
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv = now();
	long ldelay = (long) delay;
	tv.tv_sec += ldelay;
	tv.tv_usec += (long) ((delay - ldelay) * 1000000 + 0.5);
	if (tv.tv_usec >= 1000000) {
	    tv.tv_sec++;
	    tv.tv_usec -= 1000000;
	}
	at_time(tv, e);
    }
}

} // namespace tamer
