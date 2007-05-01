#ifndef TAMER__BASE_HH
#define TAMER__BASE_HH 1
#include <stdexcept>
namespace tamer {

template <typename I1=void, typename I2=void> class rendezvous;
template <typename T1=void, typename T2=void, typename T3=void, typename T4=void> class event;
inline event<> distribute(const event<> &, const event<> &);
class driver;

class tamer_error : public std::runtime_error { public:
    explicit tamer_error(const std::string &arg)
	: runtime_error(arg) {
    }
};

namespace tamerpriv {

class simple_event;
class abstract_rendezvous;
class blockable_rendezvous;
class closure;
template <typename T1> class _unbind_rendezvous;
template <typename T1> class _bind_rendezvous;
class distribute_rendezvous;
event<> hard_distribute(const event<> &e1, const event<> &e2);

class simple_event { public:

    inline simple_event()
	: _refcount(1), _r(0), _r_name(0), _r_next(0), _r_pprev(0), _canceller(0) {
    }

    template <typename R, typename I1, typename I2>
    inline simple_event(R &r, const I1 &i1, const I2 &i2);

    template <typename R, typename I1>
    inline simple_event(R &r, const I1 &i1);

    template <typename R>
    inline simple_event(R &r);

    inline ~simple_event();

    void use() {
	_refcount++;
    }
    
    void unuse() {
	if (!--_refcount)
	    delete this;
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

    inline void simple_initialize(abstract_rendezvous *r, uintptr_t rname) {
	// NB this can be called before e has been fully initialized.
	_r = r;
	_r_name = rname;
	_r_next = 0;
	_r_pprev = 0;
    }

    inline void initialize(blockable_rendezvous *r, uintptr_t rname);
    
    uintptr_t rname() const {
	return _r_name;
    }
    
    inline void at_cancel(const event<> &c);

    inline bool complete(bool success);

  protected:

    unsigned _refcount;
    abstract_rendezvous *_r;
    uintptr_t _r_name;
    simple_event *_r_next;
    simple_event **_r_pprev;
    simple_event *_canceller;

    static simple_event *dead;

    static void make_dead();
    class initializer;
    static initializer the_initializer;
    
    friend class blockable_rendezvous;
    friend class event<void, void, void, void>;
    
};


class abstract_rendezvous { public:

    abstract_rendezvous() {
    }

    virtual inline ~abstract_rendezvous() {
    }

    virtual bool is_distribute() const {
	return false;
    }
    
    virtual void complete(uintptr_t rname, bool success) = 0;

  private:

    abstract_rendezvous(const abstract_rendezvous &);
    abstract_rendezvous &operator=(const abstract_rendezvous &);
    
};


class blockable_rendezvous : public abstract_rendezvous { public:

    blockable_rendezvous()
	: _events(0), _unblocked_next(0), _unblocked_prev(0),
	  _blocked_closure(0) {
    }
    
    virtual inline ~blockable_rendezvous();

    inline void block(closure &c, unsigned where);
    inline void unblock();
    inline void run();
    
    static blockable_rendezvous *unblocked;
    static blockable_rendezvous *unblocked_tail;
    
  private:

    simple_event *_events;
    blockable_rendezvous *_unblocked_next;
    blockable_rendezvous *_unblocked_prev;
    closure *_blocked_closure;
    unsigned _blocked_closure_blockid;

    friend class simple_event;
    friend class driver;
    
};



struct closure {

    closure()
	: _refcount(1) {
    }

    virtual ~closure() {
    }

    void use() {
	_refcount++;
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }

    virtual void _closure__activate(unsigned) = 0;

  protected:

    unsigned _refcount;
    
};


inline simple_event::~simple_event()
{
    if (_r)
	complete(false);
}

inline blockable_rendezvous::~blockable_rendezvous()
{
    // first take all events off this rendezvous, so they don't call back
    for (simple_event *e = _events; e; e = e->_r_next)
	e->_r = 0;
    // then complete events, calling their cancellers
    while (_events)
	_events->complete(false);
    
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

inline void simple_event::initialize(blockable_rendezvous *r, uintptr_t rname)
{
    // NB this can be called before e has been fully initialized.
    _r = r;
    _r_name = rname;
    _r_pprev = &r->_events;
    if (r->_events)
	r->_events->_r_pprev = &_r_next;
    _r_next = r->_events;
    r->_events = this;
}

inline bool simple_event::complete(bool success)
{
    abstract_rendezvous *r = _r;
    simple_event *canceller = _canceller;

    if (_r_pprev)
	*_r_pprev = _r_next;
    if (_r_next)
	_r_next->_r_pprev = _r_pprev;
    
    _r = 0;
    _canceller = 0;
    _r_pprev = 0;
    _r_next = 0;

    if (r)
	r->complete(_r_name, success);
    if (canceller && !success)
	canceller->complete(true);
    if (canceller)
	canceller->unuse();

    return r != 0;
}

inline void blockable_rendezvous::block(closure &c, unsigned where)
{
    assert(!_blocked_closure && &c);
    c.use();
    _blocked_closure = &c;
    _blocked_closure_blockid = where;
}

inline void blockable_rendezvous::unblock()
{
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

inline void blockable_rendezvous::run()
{
    closure *c = _blocked_closure;
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
    c->_closure__activate(_blocked_closure_blockid);
    c->unuse();
}

}}
#endif /* TAMER__BASE_HH */
