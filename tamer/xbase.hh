#ifndef TAMER__BASE_HH
#define TAMER__BASE_HH 1
#include <stdexcept>
namespace tamer {

class _event_superbase;
class abstract_rendezvous;
class _rendezvous_base;
class _closure_base;
template <typename I1=void, typename I2=void> class rendezvous;
template <typename T1=void, typename T2=void, typename T3=void, typename T4=void> class event;
template <typename T1> class _unbind_rendezvous;
template <typename T1> class _bind_rendezvous;
class _scatter_rendezvous;
inline event<> scatter(const event<> &, const event<> &);
event<> _hard_scatter(const event<> &e1, const event<> &e2);
class driver;

class _event_superbase { public:

    inline _event_superbase()
	: _refcount(1), _r(0), _r_name(0), _r_next(0), _r_pprev(0), _canceller(0) {
    }

    template <typename R, typename I1, typename I2>
    inline _event_superbase(R &r, const I1 &i1, const I2 &i2);

    template <typename R, typename I1>
    inline _event_superbase(R &r, const I1 &i1);

    template <typename R>
    inline _event_superbase(R &r);

    inline ~_event_superbase();

    void use() {
	_refcount++;
    }
    
    void unuse() {
	if (!--_refcount)
	    delete this;
    }

    typedef abstract_rendezvous *_event_superbase::*unspecified_bool_type;

    operator unspecified_bool_type() const {
	return _r ? &_event_superbase::_r : 0;
    }
    
    bool empty() const {
	return _r == 0;
    }

    inline bool is_scatterer() const;

    inline void simple_initialize(abstract_rendezvous *r, uintptr_t rname) {
	assert(!_r);
	_r = r;
	_r_name = rname;
	_r_next = 0;
	_r_pprev = 0;
    }

    inline void initialize(_rendezvous_base *r, uintptr_t rname);
    
    uintptr_t rname() const {
	return _r_name;
    }
    
    inline void at_cancel(const event<> &c);

    inline bool complete(bool success);

  protected:

    unsigned _refcount;
    abstract_rendezvous *_r;
    uintptr_t _r_name;
    _event_superbase *_r_next;
    _event_superbase **_r_pprev;
    _event_superbase *_canceller;

    static _event_superbase *dead;

    static void make_dead();
    class initializer;
    static initializer the_initializer;
    
    friend class _rendezvous_base;
    friend class event<void, void, void, void>;
    friend event<> _hard_scatter(const event<> &, const event<> &);
    
};


class abstract_rendezvous { public:

    abstract_rendezvous() {
    }

    virtual inline ~abstract_rendezvous() {
    }

    virtual bool is_scatter() const {
	return false;
    }
    
    virtual void complete(uintptr_t rname, bool success) = 0;

};


class _rendezvous_base : public abstract_rendezvous { public:

    _rendezvous_base()
	: _events(0), _unblocked_next(0), _unblocked_prev(0),
	  _blocked_closure(0) {
    }
    
    virtual inline ~_rendezvous_base();

    inline void _block(_closure_base &c, unsigned where);
    inline void _unblock();
    inline void _run();
    
    static _rendezvous_base *unblocked;
    static _rendezvous_base *unblocked_tail;
    
  private:

    _event_superbase *_events;
    _rendezvous_base *_unblocked_next;
    _rendezvous_base *_unblocked_prev;
    _closure_base *_blocked_closure;
    unsigned _blocked_closure_blockid;

    _rendezvous_base(const _rendezvous_base &);
    _rendezvous_base &operator=(const _rendezvous_base &);

    friend class _event_superbase;
    friend class driver;
    
};



struct _closure_base {

    _closure_base()
	: _refcount(1) {
    }

    virtual ~_closure_base() {
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


class tamer_error : public std::runtime_error { public:

    explicit tamer_error(const std::string &arg)
	: runtime_error(arg) {
    }

};


inline _event_superbase::~_event_superbase()
{
    if (_r)
	complete(false);
}

inline bool _event_superbase::is_scatterer() const
{
    return _r && _r->is_scatter();
}

inline _rendezvous_base::~_rendezvous_base()
{
    // first take all events off this rendezvous, so they don't call back
    for (_event_superbase *e = _events; e; e = e->_r_next)
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

inline void _event_superbase::initialize(_rendezvous_base *r, uintptr_t rname)
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

inline bool _event_superbase::complete(bool success)
{
    abstract_rendezvous *r = _r;
    _event_superbase *canceller = _canceller;

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

inline void _rendezvous_base::_block(_closure_base &c, unsigned where)
{
    assert(!_blocked_closure && &c);
    c.use();
    _blocked_closure = &c;
    _blocked_closure_blockid = where;
}

inline void _rendezvous_base::_unblock()
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

inline void _rendezvous_base::_run()
{
    _closure_base *c = _blocked_closure;
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

}
#endif /* TAMER__BASE_HH */
