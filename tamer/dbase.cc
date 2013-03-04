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

namespace tamer {
namespace tamerpriv {
timeval now;
bool now_updated;
} // namespace tamerpriv

driver *driver::main;

void initialize()
{
    if (!driver::main && getenv("TAMER_LIBEV"))
	driver::main = driver::make_libev();
    if (!driver::main && !getenv("TAMER_NOLIBEVENT"))
	driver::main = driver::make_libevent();
    if (!driver::main)
	driver::main = driver::make_tamer();
}

void cleanup()
{
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

}
