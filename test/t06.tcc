// -*- mode: c++ -*-
/* Copyright (c) 2012, Eddie Kohler
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
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <tamer/tamer.hh>

tamed void run_signals() {
    tvars {
	tamer::rendezvous<int> r(tamer::rvolatile);
	int n = 0, what;
    }
    tamer::at_signal(SIGINT, make_event(r, 1));
    tamer::at_signal(SIGUSR1, make_event(r, 2));
    tamer::at_delay(100000, make_event(r, 3));
    while (1) {
	twait(r, what);
	if (what == 1) {
	    ++n;
	    tamer::at_signal(SIGINT, make_event(r, 1));
	} else
	    break;
    }
    fprintf(stdout, "received %d SIGINT\n", n);
    fflush(stdout);
}

int main(int, char **) {
    tamer::initialize();
    run_signals();
    if (fork() == 0) {
	pid_t x = getppid();
	for (int i = 0; i < 10000; ++i) {
	    kill(x, SIGINT);
	    usleep(1);
	}
	kill(x, SIGUSR1);
	exit(0);
    }
    tamer::loop();
    tamer::cleanup();
}
