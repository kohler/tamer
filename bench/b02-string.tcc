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
#include <tamer/adapter.hh>
#include <iostream>

int loops = 5000000;

template <typename T> struct make_random {};
template <> struct make_random<int> {
    int operator()() {
        return random();
    }
};
template <> struct make_random<std::string> {
    std::string operator()() {
        char buf[100];
        sprintf(buf, "%d", (int) random());
        return std::string(buf);
    }
};

tamed template <typename T> void go() {
    tvars {
        int i;
        const int nstorage = 90481, nvalues = 1024;
        T values[1024];
        T* storage;
    }
    {
        make_random<T> r;
        for (i = 0; i < nvalues; ++i)
            values[i] = r();
        storage = new T[nstorage];
    }
    twait {
        for (i = 0; i < loops; ++i)
            tamer::at_asap(tamer::bind(make_event(storage[i % nstorage]),
                                       values[i % nvalues]));
    }
    delete[] storage;
}

int main(int, char **) {
    tamer::initialize();
    go<std::string>();
    //go<int>();
    tamer::loop();
    tamer::cleanup();
}
