#include <tamer.hh>
#include <tamer/adapter.hh>

namespace tamer {

_rendezvous_base *_rendezvous_base::unblocked;
_rendezvous_base *_rendezvous_base::unblocked_tail;

_event_superbase *_event_superbase::dead;

void _event_superbase::make_dead() {
    if (!dead)
	dead = new _event_superbase;
}

class _event_superbase::initializer { public:

    initializer() {
    }

    ~initializer() {
	if (dead)
	    dead->unuse();
    }

};

_event_superbase::initializer _event_superbase::the_initializer; 


event<> _hard_scatter(const event<> &e1, const event<> &e2) {
    if (!e1)
	return e2;
    else if (!e2)
	return e1;
    else if (e1._e->is_scatterer()) {
	_scatter_rendezvous *r = static_cast<_scatter_rendezvous *>(e1._e->_r);
	r->add_scatter(e2);
	return e1;
    } else {
	_scatter_rendezvous *r = new _scatter_rendezvous;
	r->add_scatter(e1);
	r->add_scatter(e2);
	return event<>(*r);
    }
}

}
