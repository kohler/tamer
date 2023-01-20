// -*- mode: c++ -*-
/* Copyright (c) 2023, Eddie Kohler
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
#include <cstdio>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
using namespace tamer;
static double dstart;

tamed void child(struct sockaddr_in* saddr, socklen_t saddr_len) {
    tamed {
        tamer::fd cfd;
        int ret;
    }
    cfd = tamer::fd::socket(AF_INET, SOCK_STREAM, 0);
    twait {
        cfd.connect((struct sockaddr*) saddr, saddr_len, make_event(ret));
    }
    twait { tamer::at_delay_msec(100, make_event()); }
    twait { cfd.write("1", make_event(ret)); }
    twait { tamer::at_delay_msec(100, make_event()); }
    cfd.shutdown(SHUT_WR);
    twait { tamer::at_delay_msec(100, make_event()); }
    cfd.close();
}

tamed void check_read(tamer::fd cfd) {
    twait {
        tamer::at_fd_read(cfd.fdnum(), tamer::add_timeout(1, make_event()));
    }
    printf("R %.06g\n", tamer::dnow() - dstart);
}

tamed void check_shutdown(tamer::fd cfd) {
    twait {
        tamer::at_fd_shutdown(cfd.fdnum(), tamer::add_timeout(1, make_event()));
    }
    printf("SD %.06g\n", tamer::dnow() - dstart);
}

tamed void parent(tamer::fd& listenfd) {
    tvars {
        tamer::fd cfd;
    }
    twait { listenfd.accept(make_event(cfd)); }
    check_read(cfd);
    check_shutdown(cfd);
}

int main(int, char *[]) {
    tamer::initialize();
    dstart = tamer::dnow();
    signal(SIGPIPE, SIG_IGN);

    tamer::fd listenfd = tamer::tcp_listen(0);
    assert(listenfd);
    struct sockaddr_in saddr;
    socklen_t saddr_len = sizeof(saddr);
    int r = getsockname(listenfd.fdnum(), (struct sockaddr*) &saddr, &saddr_len);
    assert(r == 0);

    pid_t p = fork();
    if (p != 0) {
        parent(listenfd);
    } else {
        listenfd.close();
        child(&saddr, saddr_len);
    }

    tamer::loop();
    tamer::cleanup();
    if (p != 0) {
        kill(p, 9);
        printf("Done\n");
    }
}
