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
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

tamed void runone(tamer::fd cfd, double timeout) {
    tvars {
	char buf[BUFSIZ];
	size_t rpos = 0, nread = 0, nwritten = 0;
	tamer::rendezvous<int> rv;
	tamer::event<size_t, int> reader, writer;
	int x, r;
    }

    if (timeout > 0)
	tamer::at_delay(timeout, make_event(rv, 1));

    while (cfd) {
	if (rpos < BUFSIZ && !reader) {
	    reader = make_event(rv, 2, nread, r);
	    cfd.read_once(buf + rpos, BUFSIZ - rpos, reader);
	}
	if (rpos > 0 && !writer) {
	    writer = make_event(rv, 3, nwritten, r);
	    cfd.write_once(buf, rpos, writer);
	}

	twait(rv, x);

	if (x == 1)
	    cfd.close();
	else if (x == 2 && nread) {
	    rpos += nread;
	    writer.unblock();
	} else if (x == 3 && nwritten) {
	    rpos += nread;
	    memmove(buf, buf + nwritten, rpos - nwritten);
	    rpos -= nwritten;
	    nread = 0;
	    reader.unblock();
	}
    }

    rv.clear();
}

tamed void run(tamer::fd listenfd, double timeout) {
    tvars { tamer::fd cfd; };
    if (!listenfd)
	fprintf(stderr, "listen: %s\n", strerror(-listenfd.error()));
    while (listenfd) {
	twait { listenfd.accept(make_event(cfd)); }
	runone(cfd, timeout);
    }
}

static void usage() {
    fprintf(stderr, "Usage: tamer-echosrv [-p PORT] [-t TIMEOUT]\n");
}

int main(int argc, char **argv) {
    int opt;
    int port = 11111;
    double timeout = 0;
    while ((opt = getopt(argc, argv, "hp:t:")) != -1) {
	switch (opt) {
	case 'h':
	    usage();
	    exit(0);
	case 'p':
	    port = strtol(optarg, 0, 0);
	    break;
	case 't':
	    timeout = strtod(optarg, 0);
	    break;
	case '?':
	usage:
	    usage();
	    exit(1);
	}
    }

    if (optind != argc)
	goto usage;

    tamer::initialize();
    run(tamer::tcp_listen(port), timeout);
    tamer::loop();
    tamer::cleanup();
}
