// -*- mode: c++ -*-
/* Copyright (c) 2007-2012, Eddie Kohler
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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <tamer/tamer.hh>
#include <tamer/adapter.hh>

tamed void delay1(int x, tamer::event<> e) {
    twait { tamer::at_delay_msec(x, make_event()); }
    fprintf(stderr, "@%d: %ld.%06d\n", x, (long int) tamer::recent().tv_sec, (int) tamer::recent().tv_usec);
    e.trigger();
}

tamed void stdiny() {
    tvars { int fail = -1; }
    twait {
	tamer::at_fd_read(0, with_timeout(400, make_event(), fail));
	//tamer::at_fd_read(0, with_signal(SIGINT, make_event(), fail));
	//tamer::at_fd_read(0, make_event());
    }
    fprintf(stderr, "====stdin result %d\n", fail);
}

tamed void waitr(tamer::rendezvous<int> &r) {
    tvars { int x (); }
    fprintf(stderr, "====waitr %d\n", x);
    twait(r, x);
    fprintf(stderr, "====waitr %d\n", x);
}

tamed void waitsig() {
    tvars { int i (0); }
    while (i < 3) {
	twait {
	    tamer::at_signal(SIGINT, make_event());
	}
	fprintf(stderr, "== sig %d\n", i);
	i++;
    }
    fprintf(stderr, "== exit\n");
}

tamed void scatter() {
    tvars { int i(1), j; tamer::rendezvous<int> r; tamer::event<> e, e2; }
    tamer::at_delay_msec(500, e = tamer::distribute(TAMER_MAKE_ANNOTATED_EVENT(r, i),
						    TAMER_MAKE_ANNOTATED_EVENT(r, 2),
						    TAMER_MAKE_ANNOTATED_EVENT(r, 3)));
    tamer::at_delay_msec(502, tamer::distribute(TAMER_MAKE_ANNOTATED_EVENT(r, 0), e));
    j = 0;
    while (r.has_events()) {
	twait(r, i);
	fprintf(stderr, "@%d: distributed %d\n", i ? 500 : 502, i);
	assert(i == "1230"[j] - '0');
	++j;
    }
    // should print "== distributed 1, 2, 3, 0"

    tamer::at_delay_msec(500, e = tamer::distribute(TAMER_MAKE_ANNOTATED_EVENT(r, 1),
						    TAMER_MAKE_ANNOTATED_EVENT(r, 2),
						    TAMER_MAKE_ANNOTATED_EVENT(r, 3)));
    (void) tamer::distribute(e, TAMER_MAKE_ANNOTATED_EVENT(r, 0));
    // should print "avoided leak of active event" (because we just threw away
    // that event), then "== distributed 1, 2, 3, 0"
    j = 0;
    while (r.has_events()) {
	twait(r, i);
	fprintf(stderr, "@%d: distributed %d\n", 502, i);
	assert(i == "1230"[j] - '0');
	++j;
    }
}

tamed void cancellina() {
    tvars { int i(1), j; tamer::rendezvous<int> r; tamer::event<> e; }
    tamer::at_delay_msec(500, e = TAMER_MAKE_ANNOTATED_EVENT(r, 1));
    e.at_trigger(TAMER_MAKE_ANNOTATED_EVENT(r, 100));
    e.at_trigger(TAMER_MAKE_ANNOTATED_EVENT(r, 101));
    e.at_trigger(TAMER_MAKE_ANNOTATED_EVENT(r, 102));
    e.trigger();
    j = 0;
    while (r.has_events()) {
	twait(r, i);
	fprintf(stderr, "@0: triggered %d\n", i);
	assert(i == "\001\144\145\146"[j]);
	++j;
    }
}

tamed void cancellina2() {
    tvars { std::pair<int, int> ij = std::make_pair(1, 1);
        int k;
        tamer::rendezvous<std::pair<int, int> > r; tamer::event<> e1;
	tamer::event<> e2; tamer::event<> e3; tamer::event<> e4; }
    e1 = TAMER_MAKE_ANNOTATED_EVENT(r, std::make_pair(1, 1));
    e2 = TAMER_MAKE_ANNOTATED_EVENT(r, std::make_pair(2, 2));
    e3 = TAMER_MAKE_ANNOTATED_EVENT(r, std::make_pair(3, 3));
    e4 = tamer::distribute(e1, e2, e3);
    tamer::at_delay_msec(500, e4);
    e1.trigger();
    e2.trigger();
    e3.trigger();
    if (e4)
	fprintf(stderr, "@0: SHOULD NOT HAVE e4\n");
    assert(!e4);
    k = 0;
    while (r.has_events()) {
	twait(r, ij);
	fprintf(stderr, "@0: triggered %d %d\n", ij.first, ij.second);
	assert(ij.first == ij.second);
	assert(ij.first == "123"[k] - '0');
	++k;
    }
    fprintf(stderr, "@0: done cancellina2\n");
}

tamed void bindery() {
    tvars { int i(1); tamer::rendezvous<> r; tamer::event<int> e;
	tamer::event<> ee; }
    twait {
	tamer::at_delay_msec(600, tamer::bind(make_event(i), 2));
	fprintf(stderr, "@0: before bindery %d\n", i);
    }
    fprintf(stderr, "@600: after bindery %d\n", i);
    tamer::at_delay_msec(600, tamer::bind(e = TAMER_MAKE_ANNOTATED_EVENT(r, i), 3));
    e.unblocker().trigger();
    while (r.has_events()) {
	twait(r);
	fprintf(stderr, "@600: got bindery\n");
    }
    fprintf(stderr, "@600: after bindery 2 %d\n", i);
    // should print "after bindery 2 2", b/c e.unblocker() was triggered,
    // avoiding the bind
    assert(i == 2);
    tamer::at_delay_msec(600, ee = tamer::bind(TAMER_MAKE_ANNOTATED_EVENT(r, i), 4));
    ee.unblocker().trigger();
    while (r.has_events()) {
	twait(r);
	fprintf(stderr, "@600: got bindery\n");
    }
    fprintf(stderr, "@600: after bindery 3 %d\n", i);
    // should print "after bindery 3 4", b/c ee.unblocker() == ee was
    // triggered, affecting the bind
    assert(i == 4);

    twait { tamer::at_delay_usec(1, tamer::bind(tamer::rebind<std::string>(tamer::make_event()), "Hello")); }
}

int add1(int x) {
    return x + 1;
}

tamed void mappery() {
    tvars { int i(1); tamer::rendezvous<> r; tamer::event<int> e;
	tamer::event<> ee; }
    twait {
	tamer::at_delay_msec(700, tamer::bind(tamer::map<int>(make_event(i), add1), 2));
	fprintf(stderr, "@0: before mappery %d\n", i);
    }
    fprintf(stderr, "@700: after mappery %d\n", i); // should be 3
    assert(i == 3);
    twait {
	tamer::at_delay_msec(700, tamer::bind(tamer::map<int>(e = make_event(i), add1), 4));
	e.unblocker().trigger();
	fprintf(stderr, "@700: before mappery %d\n", i);
    }
    fprintf(stderr, "@700: after mappery %d\n", i); // should be 3, since the unblocker executed
    assert(i == 3);
    twait {
	tamer::at_asap(tamer::bind(tamer::map<int>(e = make_event(i), add1), 10));
    }
    fprintf(stderr, "@700+: after mappery %d\n", i); // should be 11
    assert(i == 11);
}

tamed void test_debug2(tamer::event<> a) {
    twait {
	tamer::at_asap(make_event());
    }
}

tamed void test_debug1() {
    twait {
	test_debug2(make_event());
    }
}

tamed void test_distribute() {
    tvars { int i, j, k, l; };
    twait {
	tamer::event<int> e1 = make_event(i);
	tamer::event<int> e2 = make_event(j);
	tamer::event<int> e3 = make_event(k);
	tamer::at_asap(tamer::bind(tamer::distribute(e1, e2, e3), 10));
	tamer::at_asap(tamer::bind(e1, 1));
	tamer::at_asap(tamer::bind(e2, 2));
	tamer::at_asap(tamer::bind(e3, 3));
	e1.trigger(1);
    }
    assert(i == 1 && j == 10 && k == 10);
    i = j = k = l = 0;
    twait {
	tamer::event<int> ei = make_event(i),
	    ej = make_event(j);
	tamer::event<int> e1 = tamer::distribute(ei, ej);
	e1.at_trigger(tamer::bind(make_event(k), 1));
	e1 = tamer::distribute(TAMER_MOVE(e1), make_event(l));
	assert(i == 0 && j == 0 && k == 0 && l == 0);
	ei.trigger(1);
	ej.trigger(1);
	assert(i == 1 && j == 1 && k == 1 && l == 0);
	e1.trigger(2);
	assert(i == 1 && j == 1 && k == 1 && l == 2);
    }
}

tamed void test_distribute_opt() {
    tvars { int i, j, k, l; };
    twait {
	tamer::event<> e1;
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        e1 = tamer::distribute(TAMER_MOVE(e1), make_event());
        //fprintf(stderr, "%d\n", static_cast<tamer::tamerpriv::distribute_rendezvous<>*>(e1.__get_simple()->rendezvous())->nchildren());
        e1();
    }
    twait {
        tamer::event<> e1 = make_event();
        e1.at_trigger(make_event());
        e1.at_trigger(make_event());
        e1.at_trigger(make_event());
        e1.at_trigger(make_event());
        e1.at_trigger(make_event());
        e1.at_trigger(make_event());
        e1.at_trigger(make_event());
        e1.at_trigger(make_event());
        e1();
    }
}

tamed void test_unnamed_param(int) {
}

tamed void test_unnamed_param1(int, int) {
}

tamed void test_unnamed_param2(int a, int) {
    assert(a == 1);
}

tamed void test_unnamed_param3(int, int a) {
    assert(a == 2);
}

tamed void test_default_param(int, int b = 2) {
    assert(b == 2);
}

tamed void test_tamed_tvars() {
    tamed { int x = 0; }
    twait { tamer::at_delay(0.01, tamer::bind(make_event(x), 1)); }
    assert(x == 1);
}

int main(int, char **) {
    tamer::rendezvous<int> r;
    tamer::initialize();
    waitr(r);
    delay1(100, tamer::event<>());
    delay1(1000, TAMER_MAKE_ANNOTATED_EVENT(r, 10));
    delay1(250, tamer::event<>());
    delay1(400, tamer::event<>());
    scatter();

    // The cancellina() tests trigger events and then immediately wait for
    // them, so they should not block.
    cancellina();
    cancellina2();

    bindery();
    mappery();

    fcntl(0, F_SETFD, O_NONBLOCK);
    stdiny();
    //waitsig();

    tamer::loop();

    test_debug1();
    // check rebinding with std::string on empty events
    tamer::at_delay_usec(1, tamer::bind(tamer::rebind<std::string>(tamer::event<>()), "Hello"));

    test_distribute();
    test_distribute_opt();

    test_unnamed_param(1);
    test_unnamed_param1(1, 2);
    test_unnamed_param2(1, 2);
    test_unnamed_param3(1, 2);
    test_default_param(1);
    test_default_param(1, 2);
    test_tamed_tvars();

    tamer::loop();
    tamer::cleanup();
}
