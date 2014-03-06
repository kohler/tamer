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
#include <tamer/fd.hh>
using tamer::event;

tamed void f(event<> e) {
    std::cout << "f\n";
    e();
}


tamed template <typename T>
void tf(T x, event<> e) {
    std::cout << "tf " << x << "\n";
    e();
}


class klass {
  public:
    tamed void f(event<> e);
    tamed template <typename T> void tf(T x, event<> e);
};

tamed void klass::f(event<> e) {
    std::cout << "klass f\n";
    e();
}

tamed template <typename T> void klass::tf(T x, event<> e) {
    std::cout << "klass tf " << x << std::endl;
    e();
}


template <typename T> class tklass {
  public:
    tklass(T x) : x_(x) {}
    tamed void f(event<> e);
    tamed void g(event<> e);
    tamed template <typename U> void tf(U x, event<> e);
    T x_;
};

tamed template <typename T> void tklass<T>::f(event<> e) {
    std::cout << "tklass f " << x_ << "\n";
    e();
}

tamed template <typename T> void tklass<T>::g(event<> e) {
    tamed { T y = x_; }
    std::cout << "tklass g " << y << "\n";
    e();
}

tamed template <typename T> template <typename U>
void tklass<T>::tf(U x, event<> e) {
    std::cout << "tklass tf " << x_ << " " << x << std::endl;
    e();
}


tamed void run() {
    tvars { klass k; tklass<int> tk(2); }
    twait { f(make_event()); }
    twait { tf(1, make_event()); }
    twait { tf("Another one", make_event()); }
    twait { k.f(make_event()); }
    twait { tk.f(make_event()); }
    twait { k.tf(1, make_event()); }
    twait { k.tf("Another one", make_event()); }
    twait { tk.tf(1, make_event()); }
    twait { tk.tf("Another one", make_event()); }
    twait { tk.g(make_event()); }
}

int main(int, char**) {
    tamer::initialize();
    run();
    tamer::loop();
    tamer::cleanup();
    std::cout << "Done!\n";
}
