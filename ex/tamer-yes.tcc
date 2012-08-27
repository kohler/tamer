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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <tamer/tamer.hh>

static sig_atomic_t count = 0;

void sigint(int) {
    fprintf(stderr, "%d\n", count);
    exit(1);
}

tamed void run(int fd, std::string s, double rate, unsigned long limit) {
    tvars { size_t w; char *xx = 0; }
    ssize_t x;

    while (1) {
	// Write a copy of `s` to `fd`. Normally written as
	// twait { fd.write(s, make_event()); }
	w = 0;
	while (w < s.length()) {
	    x = ::write(fd, s.data() + w, s.length() - w);
	    if (x != 0 && x != -1)
		w += x;
	    else if (x == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
		twait { tamer::at_fd_write(fd, make_event()); }
	    else if (x == -1 && errno == EINTR)
		/* just write again */;
	    else // exit on error
		exit(0);
	}

	++count;
	if (limit != 0 && (unsigned long) count == limit)
	    exit(0);

	// If we have a rate limit, obey it.
	if (rate > 0)
	    twait { tamer::at_delay(1 / rate, make_event()); }
    }
}

static void usage() {
    fprintf(stderr, "Usage: tamer-yes [-h] [-n] [-r RATE] [-l LIMIT] [STRING]\n");
}

int main(int argc, char **argv) {
    int opt;
    bool newline = true;
    double rate = 0;
    unsigned long limit = 0;
    while ((opt = getopt(argc, argv, "hnr:l:")) != -1) {
	switch (opt) {
	case 'h':
	    usage();
	    exit(0);
	case 'n':
	    newline = false;
	    break;
	case 'r':
	    rate = strtod(optarg, 0);
	    break;
	case 'l':
	    limit = strtoul(optarg, 0, 0);
	    break;
	case '?':
	usage:
	    usage();
	    exit(1);
	}
    }

    std::string s;
    if (optind < argc - 1)
	goto usage;
    else if (optind == argc - 1)
	s = argv[optind];
    else
	s = "y";
    if (newline)
	s += '\n';

    signal(SIGINT, sigint);
    tamer::initialize();
    // Real programs should use tamer::fd methods, such as
    // tamer::fd::make_nonblocking. This is here for minimality
    fcntl(STDOUT_FILENO, F_SETFL, fcntl(STDOUT_FILENO, F_GETFL) | O_NONBLOCK);
    run(STDOUT_FILENO, s, rate, limit);
    tamer::loop();
    tamer::cleanup();
}
