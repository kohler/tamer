#include <tamer.hh>
#include <tamer/adapter.hh>

namespace tamer {

blockable_rendezvous *blockable_rendezvous::unblocked;
blockable_rendezvous *blockable_rendezvous::unblocked_tail;

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


event<> _hard_scatter(const event<> &e1, const event<> &e2) {
    if (e1.empty())
	return e2;
    else if (e2.empty())
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
