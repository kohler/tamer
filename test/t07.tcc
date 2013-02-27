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
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <tamer/tamer.hh>

template <typename T>
struct foo {
    tamed void doit(tamer::event<> e);
};

tamed template <typename T>
void foo<T>::doit(tamer::event<> e) {
    tamer::at_asap(e);
}

tamed void function() {
    tvars { foo<int> f; }
    twait { f.doit(make_event()); }
    std::cout << "It happened\n";
}

int main(int, char **) {
    tamer::initialize();
    function();
    tamer::loop();
    tamer::cleanup();
}
