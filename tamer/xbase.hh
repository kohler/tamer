#ifndef TAMER__BASE_HH
#define TAMER__BASE_HH 1
#include <stdexcept>
namespace tamer {

#if __GNUC__
#define TAMER_CLOSUREVARATTR __attribute__((unused))
#else
#define TAMER_CLOSUREVARATTR
#endif

template <typename I0=void, typename I1=void> class rendezvous;
template <typename T0=void, typename T1=void, typename T2=void, typename T3=void> class event;
inline event<> distribute(const event<> &, const event<> &);
inline event<> distribute(const event<> &, const event<> &, const event<> &);
template <typename T0> event<T0> unbind(const event<> &);
class driver;
class gather_rendezvous;

class tamer_error : public std::runtime_error { public:
    explicit tamer_error(const std::string &arg)
	: runtime_error(arg) {
    }
};

struct no_slot {
};

namespace tamerpriv {

class simple_event;
class abstract_rendezvous;
class tamer_closure;
class distribute_rendezvous;
event<> hard_distribute(const event<> &e1, const event<> &e2);

class simple_event { public:

    // DO NOT derive from this class!
    
    inline simple_event()
	: _refcount(1), _r(0), _rid(0), _r_next(0), _r_pprev(0),
	  _at_trigger(0) {
    }

    template <typename R, typename I0, typename I1>
    inline simple_event(R &r, const I0 &i0, const I1 &i1);

    template <typename R, typename I0>
    inline simple_event(R &r, const I0 &i0);

    template <typename R>
    inline simple_event(R &r);

    static simple_event *make_dead() {
	if (!dead)
	    __make_dead();
	dead->use();
	return dead;
    }

    void use() {
	++_refcount;
    }
    
    void unuse() {
	if (--_refcount == 0) {
	    if (_r)
		trigger();
	    delete this;
	}
    }

    unsigned refcount() const {
	return _refcount;
    }

    typedef abstract_rendezvous *simple_event::*unspecified_bool_type;

    operator unspecified_bool_type() const {
	return _r ? &simple_event::_r : 0;
    }
    
    bool empty() const {
	return _r == 0;
    }

    abstract_rendezvous *rendezvous() const {
	return _r;
    }

    inline void initialize(abstract_rendezvous *r, uintptr_t rid);
    
    uintptr_t rid() const {
	return _rid;
    }

    inline bool trigger();

    inline void at_trigger(const event<> &e);

  protected:

    unsigned _refcount;
    abstract_rendezvous *_r;
    uintptr_t _rid;
    simple_event *_r_next;
    simple_event **_r_pprev;
    simple_event *_at_trigger;

    simple_event(const simple_event &);

    simple_event &operator=(const simple_event &);
    
    inline ~simple_event() {
	assert(!_r && !_r_pprev);
    }
    
    static simple_event *dead;

    static void __make_dead();
    class initializer;
    static initializer the_initializer;
    
    friend class abstract_rendezvous;
    friend class event<void, void, void, void>;
    
};


class abstract_rendezvous { public:

    abstract_rendezvous()
	: _events(0), _unblocked_next(0), _unblocked_prev(0),
	  _blocked_closure(0) {
    }
    
    virtual inline ~abstract_rendezvous();

    virtual void complete(uintptr_t rid) = 0;

    virtual bool is_distribute() const {
	return false;
    }

    virtual tamer_closure *linked_closure() const {
	return _blocked_closure;
    }

    tamer_closure *blocked_closure() const {
	return _blocked_closure;
    }

    inline void block(tamer_closure &c, unsigned where);
    inline void unblock();
    inline void run();

    inline void remove_all();
    
    static abstract_rendezvous *unblocked;
    static abstract_rendezvous *unblocked_tail;

  private:

    simple_event *_events;
    abstract_rendezvous *_unblocked_next;
    abstract_rendezvous *_unblocked_prev;
    tamer_closure *_blocked_closure;
    unsigned _blocked_closure_blockid;

    abstract_rendezvous(const abstract_rendezvous &);
    abstract_rendezvous &operator=(const abstract_rendezvous &);

    friend class simple_event;
    friend class driver;
    
};


struct tamer_closure {

    tamer_closure()
	: _refcount(1) {
    }

    virtual ~tamer_closure() {
    }

    void use() {
	_refcount++;
    }

    void unuse() {
	if (--_refcount == 0)
	    delete this;
    }

    virtual void tamer_closure_activate(unsigned) = 0;

    virtual bool block_landmark(const char *&file, unsigned &line);

  protected:

    unsigned _refcount;
    
};


struct tamer_debug_closure : public tamer_closure {

    tamer_debug_closure()
	: _block_file(0), _block_line(0) {
    }

    bool block_landmark(const char *&file, unsigned &line);

    void set_block_landmark(const char *file, unsigned line) {
	_block_file = file;
	_block_line = line;
    }

  protected:

    const char *_block_file;
    unsigned _block_line;
    
};


namespace message {
void event_prematurely_dereferenced(simple_event *e, abstract_rendezvous *r);
}


inline void abstract_rendezvous::remove_all() {
    for (simple_event *e = _events; e; e = e->_r_next)
	e->_r = 0;
    while (_events)
	_events->trigger();
}

inline abstract_rendezvous::~abstract_rendezvous() {
    // take all events off this rendezvous and call their triggerers
    remove_all();
    
    if (_unblocked_prev)
	_unblocked_prev->_unblocked_next = _unblocked_next;
    else if (unblocked == this)
	unblocked = _unblocked_next;
    if (_unblocked_next)
	_unblocked_next->_unblocked_prev = _unblocked_prev;
    else if (unblocked_tail == this)
	unblocked_tail = _unblocked_prev;
    if (_blocked_closure)
	_blocked_closure->unuse();
}

inline void simple_event::initialize(abstract_rendezvous *r, uintptr_t rid)
{
    // NB this can be called before e has been fully initialized.
    _r = r;
    _rid = rid;
    _r_pprev = &r->_events;
    if (r->_events)
	r->_events->_r_pprev = &_r_next;
    _r_next = r->_events;
    r->_events = this;
}

inline bool simple_event::trigger() {
    abstract_rendezvous *r = _r;
    simple_event *at_trigger = _at_trigger;

    if (_r_pprev)
	*_r_pprev = _r_next;
    if (_r_next)
	_r_next->_r_pprev = _r_pprev;
    
    _r = 0;
    _at_trigger = 0;
    _r_pprev = 0;
    _r_next = 0;

    if (r && _refcount == 0)
	message::event_prematurely_dereferenced(this, r);
    if (r)
	r->complete(_rid);
    if (at_trigger) {
	at_trigger->trigger();
	at_trigger->unuse();
    }

    return r != 0;
}

inline void abstract_rendezvous::block(tamer_closure &c, unsigned where) {
    assert(!_blocked_closure && &c);
    c.use();
    _blocked_closure = &c;
    _blocked_closure_blockid = where;
}

inline void abstract_rendezvous::unblock() {
    if (_blocked_closure && !_unblocked_prev && unblocked != this) {
	_unblocked_next = 0;
	_unblocked_prev = unblocked_tail;
	if (unblocked_tail)
	    unblocked_tail->_unblocked_next = this;
	else
	    unblocked = this;
	unblocked_tail = this;
    }
}

inline void abstract_rendezvous::run() {
    tamer_closure *c = _blocked_closure;
    _blocked_closure = 0;
    if (_unblocked_prev)
	_unblocked_prev->_unblocked_next = _unblocked_next;
    else if (unblocked == this)
	unblocked = _unblocked_next;
    if (_unblocked_next)
	_unblocked_next->_unblocked_prev = _unblocked_prev;
    else if (unblocked_tail == this)
	unblocked_tail = _unblocked_next;
    _unblocked_next = _unblocked_prev = 0;
    c->tamer_closure_activate(_blocked_closure_blockid);
    c->unuse();
}

}}
#endif /* TAMER__BASE_HH */
