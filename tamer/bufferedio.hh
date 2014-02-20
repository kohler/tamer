#ifndef TAMER_BUFFEREDIO_HH
#define TAMER_BUFFEREDIO_HH 1
/* Copyright (c) 2007-2014, Eddie Kohler
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
#include <string>
#include <tamer/tamer.hh>
#include <tamer/fd.hh>
namespace tamer {

class buffer { public:

    buffer(size_t initial_capacity = 1024);
    ~buffer();

    void fill_until(fd f, char c, size_t max_size, size_t &out_size, event<int> done);
    void take_until(fd f, char c, size_t max_size, std::string &str, event<int> done);

  private:

    char *_buf;
    size_t _size;
    size_t _head;
    size_t _tail;

    ssize_t fill_more(fd f, const event<int> &done);

    class closure__fill_until__2fdckRkQi_;
    void fill_until(closure__fill_until__2fdckRkQi_ &);
    class closure__take_until__2fdckRSsQi_;
    void take_until(closure__take_until__2fdckRSsQi_ &);

};

}
#endif /* TAMER_BUFFEREDIO_HH */
