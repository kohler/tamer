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



class distribute_rendezvous : public abstract_rendezvous { public:

    distribute_rendezvous() {
    }

    ~distribute_rendezvous() {
    }

    void add(simple_event *e) {
	e->simple_initialize(this, 0);
    }

    bool is_distribute() const {
	return true;
    }

    void add_distribute(const event<> &e) {
	if (e)
	    _es.push_back(e);
    }
    
    void complete(uintptr_t, bool success) {
	if (success)
	    for (std::vector<event<> >::iterator i = _es.begin(); i != _es.end(); i++)
		i->trigger();
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
	    return event<>(*d);
	}
    }
}

}}
