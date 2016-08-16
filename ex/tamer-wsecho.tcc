// -*- mode: c++ -*-
/* Copyright (c) 2016 Eddie Kohler
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
#include <tamer/websocket.hh>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>

tamed void runone(tamer::fd cfd, double delay) {
    tvars {
        tamer::http_parser hp(HTTP_REQUEST);
        tamer::http_message req, res;
        tamer::websocket_parser wp(HTTP_REQUEST);
        tamer::websocket_message wreq, wres;
        std::ostringstream buf;
    }

    twait { hp.receive(cfd, make_event(req)); }
    if (!hp.ok())
        return;
    if (!tamer::websocket_handshake::is_request(req)) {
        res.status_code(501).date_header("Date", time(NULL))
            .header("Content-Type", "text/plain")
            .body("Only WebSocket connections accepted.\n");
        twait { hp.send(cfd, res, make_event()); }
        return;
    }

    tamer::websocket_handshake::response(res, req);
    twait { hp.send(cfd, res, make_event()); }

    while (cfd && wp.ok()) {
        twait { wp.receive(cfd, make_event(wreq)); }
        if (!wreq)
            break;
        std::cerr << wreq.opcode() << '\n';
        assert(wreq.opcode() == tamer::WEBSOCKET_TEXT || wreq.opcode() == tamer::WEBSOCKET_BINARY);
        wres.opcode(wreq.opcode()).body(wreq.body());
        twait { wp.send(cfd, wres, make_event()); }
    }
}

tamed void run(tamer::fd listenfd, double delay) {
    tvars { tamer::fd cfd; };
    if (!listenfd)
	fprintf(stderr, "listen: %s\n", strerror(-listenfd.error()));
    while (listenfd) {
	twait { listenfd.accept(make_event(cfd)); }
	runone(std::move(cfd), delay);
    }
}

static void usage() {
    fprintf(stderr, "Usage: tamer-wsecho [-p PORT] [-d DELAY]\n");
}

int main(int argc, char **argv) {
    int opt;
    int port = 11111;
    double delay = 0;
    while ((opt = getopt(argc, argv, "hp:t:")) != -1) {
	switch (opt) {
	case 'h':
	    usage();
	    exit(0);
	case 'p':
	    port = strtol(optarg, 0, 0);
	    break;
	case 't':
	    delay = strtod(optarg, 0);
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
    run(tamer::tcp_listen(port), delay);
    tamer::loop();
    tamer::cleanup();
}
