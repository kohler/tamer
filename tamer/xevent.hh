#ifndef TAMER_XEVENT_HH
#define TAMER_XEVENT_HH 1
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
#include <tamer/rendezvous.hh>
namespace tamer {
namespace tamerpriv {

template <typename R, typename I0, typename I1>
inline simple_event::simple_event(R &r, const I0 &i0, const I1 &i1)
    : _refcount(1)
{
    r.add(this, i0, i1);
}

template <typename R, typename I0>
inline simple_event::simple_event(R &r, const I0 &i0)
    : _refcount(1)
{
    r.add(this, i0);
}

template <typename R>
inline simple_event::simple_event(R &r)
    : _refcount(1)
{
    r.add(this);
}

}}
#endif /* TAMER_XEVENT_HH */
