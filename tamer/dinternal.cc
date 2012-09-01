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
#include "config.h"
#include "dinternal.hh"
#include <string.h>

namespace tamer {
namespace tamerpriv {

driver_asapset::~driver_asapset() {
    while (!empty())
	simple_event::unuse(ses_[head_ & capmask_]);
    delete[] ses_;
}

void driver_asapset::expand() {
    unsigned ncapmask =
	(capmask_ + 1 ? ((capmask_ + 1) * 4 - 1) : 31);
    tamerpriv::simple_event **na = new tamerpriv::simple_event *[ncapmask + 1];
    unsigned i = 0;
    for (unsigned x = head_; x != tail_; ++x, ++i)
	na[i] = ses_[x & capmask_];
    delete[] ses_;
    ses_ = na;
    capmask_ = ncapmask;
    head_ = 0;
    tail_ = i;
}

} // namespace tamerpriv
} // namespace tamer
