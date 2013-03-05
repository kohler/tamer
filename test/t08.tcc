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
#include "config.h"
#undef TAMER_DEBUG
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <tamer/tamer.hh>

#if TAMER_HAVE_PREEVENT && !TAMER_DEBUG
template <typename R>
void immediate_preevent(int i, tamer::preevent<R> e) {
    if (i % 1024 == 0)
        tamer::at_asap(e);
    else
        e();
}

tamed void function_preevent() {
    twait {
        for (int i = 0; i < 10000000; ++i)
            immediate_preevent(i, make_event());
    }
    std::cout << "It happened\n";
}
#endif

void immediate_event(int i, tamer::event<> e) {
    if (i % 1024 == 0)
        tamer::at_asap(e);
    else
        e();
}

tamed void function_event() {
    twait {
        for (int i = 0; i < 10000000; ++i)
            immediate_event(i, make_event());
    }
    std::cout << "It happened\n";
}

int main(int argc, char** argv) {
    tamer::initialize();
    if (argc == 1 || (argc > 1 && strcmp(argv[1], "preevent") == 0)) {
#if !TAMER_HAVE_PREEVENT || TAMER_DEBUG
        std::cerr << "preevent<> not supported (need C++11 && !TAMER_DEBUG)\n";
#else
        function_preevent();
#endif
    } else if (argc > 1 && strcmp(argv[1], "event") == 0)
        function_event();
    else
        std::cerr << "Usage: t08 [preevent | event]\n";
    tamer::loop();
    tamer::cleanup();
}
