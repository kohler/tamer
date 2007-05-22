#include <tamer/tamer.hh>
#include <tamer/adapter.hh>
#include <stdio.h>

namespace tamer {
namespace tamerpriv {

abstract_rendezvous *abstract_rendezvous::unblocked;
abstract_rendezvous *abstract_rendezvous::unblocked_tail;

simple_event *simple_event::dead;

void simple_event::make_dead() {
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
	// Need to disconnect all events first, so that destroying the events
	// in _es doesn't call back into distribute_rendezvous::complete.
	disconnect_all();
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
	    _es.back().at_cancel(event<>(*this, 1));
	}
    }
    
    void complete(uintptr_t rid, bool success) {
	if (success && rid) {
	    while (_es.size() && !_es.back())
		_es.pop_back();
	} else if (success) {
	    for (std::vector<event<> >::iterator i = _es.begin(); i != _es.end(); i++)
		i->trigger();
	}
	if (!rid || !_es.size())
	    delete this;
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
	if (r->is_distribute() && e1.__get_simple()->refcount() == 1) {
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

void rendezvous_dead(abstract_rendezvous *r) {
    tamer_closure *c = r->blocked_closure();
    const char *file;
    unsigned line;
    if (c->block_landmark(file, line))
	fprintf(stderr, "%s:%d: twait() may hang forever because an event was canceled\n", file, line);
    else
	fprintf(stderr, "twait() may hang forever because an event was canceled\n");
}

void gather_rendezvous_dead(gather_rendezvous *r) {
    tamer_closure *c = r->blocked_closure();
    const char *file;
    unsigned line;
    if (c->block_landmark(file, line))
	fprintf(stderr, "%s:%d: twait{} will hang forever because an event was canceled\n", file, line);
    else
	fprintf(stderr, "twait{} will hang forever because an event was canceled\n");
}

}

}}
