/* Copyright (c) 2007-2013, Eddie Kohler
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
#include <tamer/adapter.hh>
#include <stdio.h>

namespace tamer {
namespace tamerpriv {

blocking_rendezvous *blocking_rendezvous::unblocked = 0;
blocking_rendezvous **blocking_rendezvous::unblocked_ptail = &unblocked;

void blocking_rendezvous::hard_free() {
    if (unblocked_next_ != unblocked_sentinel()) {
	blocking_rendezvous **p = &unblocked;
	while (*p != this)
	    p = &(*p)->unblocked_next_;
	if (!(*p = unblocked_next_))
	    unblocked_ptail = p;
    }
    blocked_closure_->tamer_block_position_ = 1;
    blocked_closure_->tamer_activator_(blocked_closure_);
}

tamer_closure *abstract_rendezvous::linked_closure() const {
    if (rtype_ == rgather) {
	const gather_rendezvous *gr = static_cast<const gather_rendezvous *>(this);
	return gr->linked_closure_;
    } else if (rtype_ == rexplicit) {
	const blocking_rendezvous *br = static_cast<const blocking_rendezvous *>(this);
	return br->blocked_closure_;
    } else
	return 0;
}

tamer_debug_closure *abstract_rendezvous::linked_debug_closure() const {
    tamer_closure *c = linked_closure();
    if (c && int(c->tamer_block_position_) < 0)
	return static_cast<tamer_debug_closure *>(c);
    else
	return 0;
}


void simple_event::simple_trigger(simple_event *x, bool values) TAMER_NOEXCEPT {
    if (!x)
	return;
    simple_event *to_delete = 0;

 retry:
    abstract_rendezvous *r = x->_r;

    if (r) {
	// See also trigger_list_for_remove(), trigger_for_unuse().
	x->_r = 0;
	*x->_r_pprev = x->_r_next;
	if (x->_r_next)
	    x->_r_next->_r_pprev = x->_r_pprev;

	if (r->rtype_ == rgather) {
	    gather_rendezvous *gr = static_cast<gather_rendezvous *>(r);
	    if (!gr->waiting_)
		gr->unblock();
	} else if (r->rtype_ == rexplicit) {
	    explicit_rendezvous *er = static_cast<explicit_rendezvous *>(r);
	    simple_event::use(x);
	    *er->ready_ptail_ = x;
	    er->ready_ptail_ = &x->_r_next;
	    x->_r_next = 0;
	    er->unblock();
	} else {
	    // rfunctional || rdistribute
	    functional_rendezvous *fr = static_cast<functional_rendezvous *>(r);
	    fr->f_(fr, x, values);
	}
    }

    // Important to keep an event in memory until all its at_triggers are
    // triggered -- some functional rendezvous, like with_helper_rendezvous,
    // depend on this property -- so don't delete just yet.
    if (--x->_refcount == 0) {
	x->_r_next = to_delete;
	to_delete = x;
    }

    if (r && x->at_trigger_f_) {
	if (x->at_trigger_f_ == trigger_hook) {
	    x = static_cast<simple_event*>(x->at_trigger_arg_);
	    values = false;
	    goto retry;
        } else
	    x->at_trigger_f_(x->at_trigger_arg_);
    }

    while ((x = to_delete)) {
	to_delete = x->_r_next;
	delete x;
    }
}

void simple_event::trigger_list_for_remove() TAMER_NOEXCEPT {
    // first, remove all the events (in case an at_trigger() is also waiting
    // on this rendezvous)
    for (simple_event *e = this; e; e = e->_r_next)
	e->_r = 0;
    // then call any left-behind at_triggers
    for (simple_event *e = this; e; e = e->_r_next)
	if (e->at_trigger_f_)
	    e->at_trigger_f_(e->at_trigger_arg_);
}

void simple_event::trigger_hook(void* arg) {
    simple_trigger(static_cast<simple_event*>(arg), false);
}

void simple_event::unuse_trigger() TAMER_NOEXCEPT {
    if (_r) {
	message::event_prematurely_dereferenced(this, _r);
	++_refcount;
	simple_trigger(false);
    } else
	delete this;
}

inline event<> simple_event::at_trigger_event(void (*f)(void*), void* arg) {
    if (f == trigger_hook)
	return event<>::__make(static_cast<simple_event*>(arg));
    else
	return tamer::fun_event(f, arg);
}

void simple_event::hard_at_trigger(simple_event* x, void (*f)(void*),
				   void* arg) {
    if (!x || !*x)
	f(arg);
    else {
	x->at_trigger_arg_ =
	    tamer::distribute(at_trigger_event(x->at_trigger_f_,
                                               x->at_trigger_arg_),
			      at_trigger_event(f, arg))
	    .__take_simple();
	x->at_trigger_f_ = trigger_hook;
    }
}


namespace message {

void event_prematurely_dereferenced(simple_event *, abstract_rendezvous *r) {
    if (r->is_volatile())
	/* no error message */;
    else if (tamer_debug_closure *dc = r->linked_debug_closure())
	fprintf(stderr, "%s:%d: avoided leak of active event\n",
		dc->tamer_blocked_file_, dc->tamer_blocked_line_);
    else
	fprintf(stderr, "avoided leak of active event\n");
}

} // namespace tamer::tamerpriv::message
} // namespace tamer::tamerpriv

void rendezvous<>::clear()
{
    abstract_rendezvous::remove_waiting();
    explicit_rendezvous::remove_ready();
}

void gather_rendezvous::clear()
{
    abstract_rendezvous::remove_waiting();
}

} // namespace tamer
