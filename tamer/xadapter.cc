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
#include "config.h"
#include <tamer/tamer.hh>
#include <tamer/xadapter.hh>
namespace tamer {
namespace tamerpriv {

placeholder_buffer_type placeholder_buffer;

void with_helper_rendezvous::hook(functional_rendezvous *fr,
				  simple_event *, bool) TAMER_NOEXCEPT {
    with_helper_rendezvous *self = static_cast<with_helper_rendezvous *>(fr);
    if (*self->e_) {
	*self->s0_ = self->v0_;
	tamerpriv::simple_event::use(self->e_);
	self->e_->simple_trigger(false);
    } else
	*self->s0_ = int();
    delete self;
}

}}
