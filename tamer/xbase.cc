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

    distribute_rendezvous()
	: _ncomplete(0) {
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

    void add_distribute(const event<> &e, bool complete) {
	if (!e)
	    return;
	if (complete && _es.size()) {
	    _es.push_back(_es[_ncomplete]);
	    _es[_ncomplete] = e;
	} else
	    _es.push_back(e);
	if (complete)
	    _es[_ncomplete++].at_cancel(event<>(*this, 1));
	else
	    _es.back().at_cancel(event<>(*this, 1));
    }
    
    void complete(uintptr_t rid, bool success) {
	if (rid && success) {
	    while (_es.size() && !_es.back())
		_es.pop_back();
	    if (_ncomplete > _es.size())
		_ncomplete = _es.size();
	} else if (!rid) {
	    for (std::vector<event<> >::size_type i = 0;
		 i < (success ? _es.size() : _ncomplete);
		 i++)
		_es[i].trigger();
	}
	if (!rid || !_es.size())
	    delete this;
    }
    
  private:

    std::vector<event<> > _es;
    std::vector<event<> >::size_type _ncomplete;
    
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
	    d->add_distribute(e2, false);
	    return e1;
	} else {
	    distribute_rendezvous *d = new distribute_rendezvous;
	    d->add_distribute(e1, false);
	    d->add_distribute(e2, false);
	    return event<>(*d, 0);
	}
    }
}


void simple_event::at_complete(const event<> &e)
{
    if (!_r)
	e._e->complete(true);
    else if (!e)
	/* do nothing */;
    else if (!_canceler) {
	distribute_rendezvous *d = new distribute_rendezvous;
	d->add_distribute(e, true);
	_canceler = new simple_event(*d, 0);
    } else {
	abstract_rendezvous *r = _canceler->rendezvous();
	if (r->is_distribute() && _canceler->refcount() == 1) {
	    // safe to reuse _canceler
	    distribute_rendezvous *d = static_cast<distribute_rendezvous *>(r);
	    d->add_distribute(e, true);
	} else {
	    distribute_rendezvous *d = new distribute_rendezvous;
	    d->add_distribute(event<>::__take(_canceler), false);
	    d->add_distribute(e, true);
	    _canceler = new simple_event(*d, 0);
	}
    }
}


namespace {
struct cancelfunc {
    cancelfunc(simple_event *e)
	: _e(e) {
	_e->weak_use();
    }
    cancelfunc(const cancelfunc &other)
	: _e(other._e) {
	_e->weak_use();
    }
    ~cancelfunc() {
	_e->weak_unuse(); // XXX this is a bad idea!
    }
    void operator()() {
	_e->complete(false);
    }
    tamerpriv::simple_event *_e;
  private:
    cancelfunc &operator=(const cancelfunc &);
};
}

event<> simple_event::canceler() {
    if (_r) {
	function_rendezvous<cancelfunc> *r =
	    new function_rendezvous<cancelfunc>(this);
	at_cancel(event<>(*r, r->canceler));
	return event<>(*r, r->triggerer);
    } else
	return event<>();
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
