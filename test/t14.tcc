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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
#include <tamer/bufferedio.hh>
using namespace tamer;

tamed void child(struct sockaddr_in* saddr, socklen_t saddr_len) {
    tvars { tamer::fd cfd; int ret; tamer::buffer buf; size_t nwritten;
        int write_rounds = 0; int wret = 0; std::string str; }
    cfd = tamer::fd::socket(AF_INET, SOCK_STREAM, 0);
    twait { cfd.connect((struct sockaddr*) saddr, saddr_len, make_event(ret)); }
    while (cfd) {
        if (wret == 0) {
            twait { cfd.write("Hello\n", 6, nwritten, make_event(wret)); }
            if (wret == 0 && nwritten == 6) {
                ++write_rounds;
                if (write_rounds <= 6)
                    printf("W 0: Hello\n");
            } else if (wret == 0)
                wret = -ECANCELED;
        }
        twait { buf.take_until(cfd, '\n', 1024, str, make_event(ret)); }
        if (ret != 0) {
            printf("W error %s after %d\n", strerror(-wret), write_rounds);
            break;
        } else if (str.length())
            printf("R %d: %s", ret, str.c_str());
    }
    cfd.close();
}

tamed void parent(tamer::fd& listenfd) {
    tvars { tamer::fd cfd; int ret; tamer::buffer buf;
        int n = 0; std::string str; }
    twait { listenfd.accept(make_event(cfd)); }
    while (cfd && n < 6) {
        ++n;
        twait { buf.take_until(cfd, '\n', 1024, str, make_event(ret)); }
        assert(ret == 0);
        str = "Ret " + str;
        twait { cfd.write(str, make_event()); }
    }
    cfd.shutdown(SHUT_RD);
    while (cfd && n < 12) {
        ++n;
        twait { cfd.write("Heh\n", 4, make_event(ret)); }
        assert(ret == 0);
    }
    cfd.close();
    listenfd.close();
}

int main(int, char *[]) {
    tamer::initialize();
    signal(SIGPIPE, SIG_IGN);

    tamer::fd listenfd = tamer::tcp_listen(0);
    assert(listenfd);
    struct sockaddr_in saddr;
    socklen_t saddr_len = sizeof(saddr);
    int r = getsockname(listenfd.value(), (struct sockaddr*) &saddr, &saddr_len);
    assert(r == 0);

    pid_t p = fork();
    if (p != 0) {
        listenfd.close();
        child(&saddr, saddr_len);
    } else
        parent(listenfd);

    tamer::loop();
    tamer::cleanup();
    if (p != 0)
        printf("Done\n");
}
