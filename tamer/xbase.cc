/* Copyright (c) 2007-2015, Eddie Kohler
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
#include <sstream>

namespace tamer {
namespace tamerpriv {

simple_driver::simple_driver()
    : ccap_(0), cfree_(1), cunblocked_(0), cunblocked_tail_(0), cs_() {
    grow();
}

simple_driver::~simple_driver() {
    delete[] cs_;
}

void simple_driver::grow() {
    unsigned new_ccap = ccap_ ? ccap_ * 2 : 64;
    cptr* new_cs = new cptr[new_ccap];
    memcpy(new_cs, cs_, sizeof(cptr) * ccap_);
    for (unsigned i = ccap_; i != new_ccap - 1; ++i) {
        new_cs[i].c = 0;
        new_cs[i].next = i + 1;
    }
    new_cs[new_ccap - 1].c = 0;
    new_cs[new_ccap - 1].next = 0;
    cfree_ = ccap_ ? ccap_ : 1;
    ccap_ = new_ccap;
    delete[] cs_;
    cs_ = new_cs;
}

void simple_driver::add(closure* c) {
    if (!cfree_) {
        grow();
    }
    unsigned i = cfree_;
    cfree_ = cs_[i].next;
    cs_[i].c = c;
    cs_[i].next = 0;
    c->tamer_driver_index_ = i;
}


void blocking_rendezvous::hard_free() TAMER_NOEXCEPT {
    assert(blocked_closure_);
    blocked_closure_->tamer_block_position_ = (unsigned) -1;
    blocked_closure_->unblock();
}

std::string closure::location() const {
#if !TAMER_NOTRACE
    std::stringstream buf;
    if (tamer_location_file_ && tamer_location_line_) {
        buf << tamer_location_file_ << ":" << tamer_location_line_;
    } else if (tamer_location_file_) {
        buf << tamer_location_file_;
    } else if (tamer_location_line_) {
        buf << tamer_location_line_;
    } else {
        buf << "<unknown>";
    }
    return buf.str();
#else
    return std::string("<unknown>");
#endif
}

std::string closure::location_description() const {
#if !TAMER_NOTRACE
    std::stringstream buf;
    if (tamer_location_file_ && tamer_location_line_) {
        buf << tamer_location_file_ << ":" << tamer_location_line_;
    } else if (tamer_location_file_) {
        buf << tamer_location_file_;
    } else if (tamer_location_line_) {
        buf << tamer_location_line_;
    } else if (!has_description()) {
        buf << "<unknown>";
    }
    if (has_description()) {
        if (buf.tellp() != 0) {
            buf << " ";
        }
        buf << *tamer_description_;
    }
    return buf.str();
#else
    return std::string("<unknown>");
#endif
}


void simple_event::simple_trigger(simple_event* x, bool values) TAMER_NOEXCEPT {
    if (!x) {
        return;
    }
    simple_event* to_delete = 0;

 retry:
    abstract_rendezvous* r = x->_r;
    bool reduce_refcount = x->_refcount > 0;

    if (r) {
        // See also trigger_list_for_remove()
        x->_r = 0;
        *x->_r_pprev = x->_r_next;
        if (x->_r_next) {
            x->_r_next->_r_pprev = x->_r_pprev;
        }

        if (r->rtype_ == rgather) {
            gather_rendezvous* gr = static_cast<gather_rendezvous *>(r);
            if (!gr->waiting_) {
                gr->unblock();
            }
        } else if (r->rtype_ == rexplicit) {
            explicit_rendezvous* er = static_cast<explicit_rendezvous *>(r);
            simple_event::use(x);
            *er->ready_ptail_ = x;
            er->ready_ptail_ = &x->_r_next;
            x->_r_next = 0;
            er->unblock();
        } else {
            // rfunctional || rdistribute
            functional_rendezvous* fr = static_cast<functional_rendezvous *>(r);
            fr->f_(fr, x, values);
        }
    }

    // Important to keep an event in memory until all its at_triggers are
    // triggered -- some functional rendezvous, like with_helper_rendezvous,
    // depend on this property -- so don't delete just yet.
    if (reduce_refcount) {
        --x->_refcount;
    }
    if (x->_refcount == 0) {
        x->_r_next = to_delete;
        to_delete = x;
    }

    if (r && x->at_trigger_f_) {
        if (x->at_trigger_f_ == trigger_hook) {
            x = static_cast<simple_event*>(x->at_trigger_arg_);
            values = false;
            goto retry;
        } else {
            x->at_trigger_f_(x->at_trigger_arg_);
        }
    }

    while ((x = to_delete)) {
        to_delete = x->_r_next;
        delete x;
    }
}

void simple_event::trigger_list_for_remove() TAMER_NOEXCEPT {
    // first, remove all the events (in case an at_trigger() is also waiting
    // on this rendezvous)
    for (simple_event* e = this; e; e = e->_r_next) {
        e->_r = 0;
    }
    // then call any left-behind at_triggers
    for (simple_event *e = this; e; e = e->_r_next) {
        if (e->at_trigger_f_)
            e->at_trigger_f_(e->at_trigger_arg_);
    }
}

void simple_event::trigger_hook(void* arg) {
    simple_trigger(static_cast<simple_event*>(arg), false);
}

void simple_event::unuse_trigger() TAMER_NOEXCEPT {
    if (_r) {
        message::event_prematurely_dereferenced(this, _r);
        simple_trigger(false);
    } else {
        delete this;
    }
}

inline event<> simple_event::at_trigger_event(void (*f)(void*), void* arg) {
    if (f == trigger_hook) {
        return event<>::__make(static_cast<simple_event*>(arg));
    } else {
        return tamer::fun_event(f, arg);
    }
}

void simple_event::hard_at_trigger(simple_event* x, void (*f)(void*),
                                   void* arg) {
    if (x && *x) {
        event<> t(at_trigger_event(x->at_trigger_f_, x->at_trigger_arg_));
        t += at_trigger_event(f, arg);
        x->at_trigger_f_ = trigger_hook;
        x->at_trigger_arg_ = t.__release_simple();
    } else {
        f(arg);
    }
}


namespace message {

void event_prematurely_dereferenced(simple_event* se, abstract_rendezvous* r) {
    if (!r->is_volatile()) {
        fprintf(stderr, "tamer: dropping last reference to active event\n");
        if (se->file_annotation() && se->line_annotation()) {
            fprintf(stderr, "tamer:   %s:%d: event was created here\n", se->file_annotation(), se->line_annotation());
        } else if (se->file_annotation()) {
            fprintf(stderr, "tamer:   %s: event was created here\n", se->file_annotation());
        }
    }
}

} // namespace tamer::tamerpriv::message
} // namespace tamer::tamerpriv

void rendezvous<>::clear() {
    abstract_rendezvous::remove_waiting();
    explicit_rendezvous::remove_ready();
}

void gather_rendezvous::clear() {
    abstract_rendezvous::remove_waiting();
}

void destroy_guard::birth_shiva(tamerpriv::closure& cl, tamed_class* k) {
    shiva_ = new shiva_closure;
    shiva_->linked_closure_ = &cl;
    shiva_->initialize_closure(activator, k);
    shiva_->tamer_blocked_driver_ = &tamerpriv::simple_driver::immediate_driver;
}

void destroy_guard::activator(tamerpriv::closure* cl) {
    shiva_closure* shiva = static_cast<shiva_closure*>(cl);
    shiva->linked_closure_->tamer_block_position_ = (unsigned) -1;
    shiva->linked_closure_->unblock();
}

} // namespace tamer
