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

static sig_atomic_t count = 0;

void sigint(int) {
    fprintf(stderr, "%d\n", count);
    exit(1);
}

tamed void run(tamer::fd fd, std::string s, double rate, unsigned long limit) {
    tvars { size_t w; char *xx = 0; int r; }

    while (true) {
        // Write a copy of `s` to `fd`
        twait { fd.write(s, make_event(r)); }
        // Just exit on error
        if (r < 0)
            exit(0);

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
    tamer::fd fd(STDOUT_FILENO);
    fd.make_nonblocking();
    run(fd, s, rate, limit);
    tamer::loop();
    tamer::cleanup();
}
