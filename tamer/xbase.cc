/* Copyright (c) 2007, Eddie Kohler
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
#include <tamer/tamer.hh>
#include <tamer/adapter.hh>
#include <stdio.h>
namespace tamer {
namespace tamerpriv {

abstract_rendezvous *abstract_rendezvous::unblocked;
abstract_rendezvous *abstract_rendezvous::unblocked_tail;

simple_event *simple_event::dead;

void simple_event::__make_dead() {
    if (!dead)
	dead = new simple_event;
}

class simple_event::initializer { public:

    initializer() {
    }

    ~initializer() {
	if (dead)
	    dead->unuse();
    }

};

simple_event::initializer simple_event::the_initializer;


bool tamer_closure::block_landmark(const char *&file, unsigned &line) {
    file = "<unknown>";
    line = 0;
    return false;
}

bool tamer_debug_closure::block_landmark(const char *&file, unsigned &line) {
    if (!_block_file)
	return tamer_closure::block_landmark(file, line);
    else {
	file = _block_file;
	line = _block_line;
	return true;
    }
}


class distribute_rendezvous : public abstract_rendezvous { public:

    distribute_rendezvous() {
    }

    ~distribute_rendezvous() {
    }

    void add(simple_event *e, uintptr_t rid) {
	e->initialize(this, rid);
    }

    bool is_distribute() const {
	return true;
    }

    void add_distribute(const event<> &e) {
	if (e) {
	    _es.push_back(e);
	    _es.back().at_trigger(event<>(*this, 1));
	}
    }

    void complete(uintptr_t rid) {
	while (_es.size() && !_es.back())
	    _es.pop_back();
	if (!_es.size() || !rid) {
	    remove_all();
	    for (std::vector<event<> >::iterator i = _es.begin(); i != _es.end(); ++i)
		i->trigger();
	    delete this;
	}
    }

  private:

    std::vector<event<> > _es;

};


event<> hard_distribute(const event<> &e1, const event<> &e2) {
    if (e1.empty())
	return e2;
    else if (e2.empty())
	return e1;
    else {
	abstract_rendezvous *r = e1.__get_simple()->rendezvous();
	if (r->is_distribute() && e1.__get_simple()->refcount() == 1
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
    const char *file;
    unsigned line;
    if (r->is_volatile())
	/* no error message */;
    else if (c && c->block_landmark(file, line))
	fprintf(stderr, "%s:%d: avoided leak of active event\n", file, line);
    else
	fprintf(stderr, "avoided leak of active event\n");
}

}

}}
