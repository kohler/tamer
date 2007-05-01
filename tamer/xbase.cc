#include <tamer.hh>
#include <tamer/adapter.hh>

namespace tamer {
namespace tamerpriv {

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


event<> hard_distribute(const event<> &e1, const event<> &e2) {
    if (e1.empty())
	return e2;
    else if (e2.empty())
	return e1;
    else if (e1.__get_simple()->is_distributor()) {
	distribute_rendezvous *r = static_cast<distribute_rendezvous *>(e1.__get_simple()->rendezvous());
	r->add_distribute(e2);
	return e1;
    } else {
	distribute_rendezvous *r = new distribute_rendezvous;
	r->add_distribute(e1);
	r->add_distribute(e2);
	return event<>(*r);
    }
}

}}
