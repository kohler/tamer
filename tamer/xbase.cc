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
#include "config.h"
#include <tamer/tamer.hh>
#include <tamer/adapter.hh>
#include <stdio.h>
namespace tamer {
namespace tamerpriv {

abstract_rendezvous *abstract_rendezvous::unblocked = 0;
abstract_rendezvous **abstract_rendezvous::unblocked_ptail = &unblocked;

void abstract_rendezvous::hard_free() {
    if (unblocked_next_ != unblocked_sentinel()) {
	abstract_rendezvous **p = &unblocked;
	while (*p != this)
	    p = &(*p)->unblocked_next_;
	if (!(*p = unblocked_next_))
	    unblocked_ptail = p;
    }
    _blocked_closure->tamer_block_position_ = 1;
    _blocked_closure->tamer_activator_(_blocked_closure);
}

tamer_closure *abstract_rendezvous::linked_closure() const {
    if (rtype_ == rgather) {
	const gather_rendezvous *gr = static_cast<const gather_rendezvous *>(this);
	return gr->linked_closure_;
    } else
	return _blocked_closure;
}


class distribute_rendezvous : public abstract_rendezvous {
  public:
    distribute_rendezvous()
	: abstract_rendezvous(rnormal, rdistribute) {
    }
    void add(simple_event *e, uintptr_t rid) {
	e->initialize(this, rid);
    }
    void add_distribute(const event<> &e) {
	if (e) {
	    es_.push_back(e);
	    es_.back().at_trigger(event<>(*this, 1));
	}
    }
    void complete(simple_event *e);
  private:
    std::vector<event<> > es_;
};

void distribute_rendezvous::complete(simple_event *e) {
    while (es_.size() && !es_.back())
	es_.pop_back();
    if (!es_.size() || !e->rid()) {
	remove_waiting();
	for (std::vector<event<> >::iterator i = es_.begin();
	     i != es_.end();
	     ++i)
	    i->trigger();
	delete this;
    }
}


void simple_event::simple_trigger(bool values) {
    simple_event *x = this;
    simple_event *to_delete = 0;

 retry:
    abstract_rendezvous *r = x->_r;
    simple_event *next = x->_at_trigger;

    if (r) {
	// See also trigger_list_for_remove(), trigger_for_unuse().
	x->_r = 0;
	*x->_r_pprev = x->_r_next;
	if (x->_r_next)
	    x->_r_next->_r_pprev = x->_r_pprev;

	if (r->rtype_ == rgather) {
	    if (!r->waiting_)
		r->unblock();
	} else if (r->rtype_ == rexplicit) {
	    explicit_rendezvous *er = static_cast<explicit_rendezvous *>(r);
	    simple_event::use(x);
	    *er->ready_ptail_ = x;
	    er->ready_ptail_ = &x->_r_next;
	    x->_r_next = 0;
	    er->unblock();
	} else if (r->rtype_ == rfunctional) {
	    functional_rendezvous *fr = static_cast<functional_rendezvous *>(r);
	    fr->f_(fr, x, values);
	} else if (r->rtype_ == rdistribute) {
	    distribute_rendezvous *dr = static_cast<distribute_rendezvous *>(r);
	    dr->complete(x);
	}
    }

    // Important to keep an event in memory until all its at_triggers are
    // triggered -- some functional rendezvous, like with_helper_rendezvous,
    // depend on this property -- so don't delete just yet.
    if (--x->_refcount == 0) {
	x->_r_next = to_delete;
	to_delete = x;
    }

    if (r && next) {
	x = next;
	values = false;
	goto retry;
    }

    while ((x = to_delete)) {
	to_delete = x->_r_next;
	delete x;
    }
}

void simple_event::trigger_list_for_remove() {
    // first, remove all the events (in case an at_trigger() is also waiting
    // on this rendezvous)
    for (simple_event *e = this; e; e = e->_r_next)
	e->_r = 0;
    // then call any left-behind at_triggers
    for (simple_event *e = this; e; e = e->_r_next)
	if (simple_event *t = e->_at_trigger)
	    t->simple_trigger(false);
}

void simple_event::trigger_for_unuse() {
#if TAMER_DEBUG
    assert(_r && _refcount == 0);
#endif
    message::event_prematurely_dereferenced(this, _r);
    _refcount = 2;		// to prevent a recursive call to unuse
    simple_trigger(false);
    _refcount = 0;		// restore old _refcount
}


event<> hard_distribute(const event<> &e1, const event<> &e2) {
    if (e1.empty())
	return e2;
    else if (e2.empty())
	return e1;
    else {
	abstract_rendezvous *r = e1.__get_simple()->rendezvous();
	if (r->rtype() == rdistribute
	    && e1.__get_simple()->refcount() == 1
	    && e1.__get_simple()->has_at_trigger()) {
	    // safe to reuse e1
	    distribute_rendezvous *d = static_cast<distribute_rendezvous *>(r);
	    d->add_distribute(e2);
	    return e1;
	} else {
	    distribute_rendezvous *d = new distribute_rendezvous;
	    d->add_distribute(e1);
	    d->add_distribute(e2);
	    return event<>(*d, 0);
	}
    }
}


namespace message {

void event_prematurely_dereferenced(simple_event *, abstract_rendezvous *r) {
    tamer_closure *c = r->linked_closure();
    if (r->is_volatile())
	/* no error message */;
    else if (c && int(c->tamer_block_position_) < 0) {
	tamer_debug_closure *dc = static_cast<tamer_debug_closure *>(c);
	fprintf(stderr, "%s:%d: avoided leak of active event\n",
		dc->tamer_blocked_file_, dc->tamer_blocked_line_);
    } else
	fprintf(stderr, "avoided leak of active event\n");
}

}

}}
