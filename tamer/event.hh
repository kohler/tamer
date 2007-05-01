#ifndef TAMER__EVENT_HH
#define TAMER__EVENT_HH 1
#include <tamer/_rendezvous.hh>
namespace tamer {

template <typename R, typename I1, typename I2>
_event_superbase::_event_superbase(R &r, const I1 &i1, const I2 &i2)
    : _refcount(1), _canceller(0)
{
    r.add(this, i1, i2);
}

template <typename R, typename I1>
_event_superbase::_event_superbase(R &r, const I1 &i1)
    : _refcount(1), _canceller(0)
{
    r.add(this, i1);
}

template <typename R>
inline _event_superbase::_event_superbase(R &r)
    : _refcount(1), _canceller(0)
{
    r.add(this);
}


template <typename T1, typename T2, typename T3, typename T4>
class _event_base : public _event_superbase { public:

    template <typename R, typename I1, typename I2>
    _event_base(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _event_superbase(r, i1, i2), _t1(&t1), _t2(&t2), _t3(&t3), _t4(&t4) {
    }

    template <typename R, typename I1>
    _event_base(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _event_superbase(r, i1), _t1(&t1), _t2(&t2), _t3(&t3), _t4(&t4) {
    }

    template <typename R>
    _event_base(R &r, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _event_superbase(r), _t1(&t1), _t2(&t2), _t3(&t3), _t4(&t4) {
    }

    void unuse() { if (!--_refcount) delete this; }
    
    void trigger(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
	if (_event_superbase::complete(true)) {
	    if (_t1) *_t1 = t1;
	    if (_t2) *_t2 = t2;
	    if (_t3) *_t3 = t3;
	    if (_t4) *_t4 = t4;
	}
    }

    void unbind() {
	_t1 = 0;
	_t2 = 0;
	_t3 = 0;
	_t4 = 0;
    }
    
  private:

    T1 *_t1;
    T2 *_t2;
    T3 *_t3;
    T4 *_t4;

    friend class event<T1, T2, T3, T4>;
    
};


template <typename T1, typename T2, typename T3>
class _event_base<T1, T2, T3, void> : public _event_superbase { public:

    template <typename R, typename I1, typename I2>
    _event_base(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3)
	: _event_superbase(r, i1, i2), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    template <typename R, typename I1>
    _event_base(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3)
	: _event_superbase(r, i1), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    template <typename R>
    _event_base(R &r, T1 &t1, T2 &t2, T3 &t3)
	: _event_superbase(r), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    void unuse() { if (!--_refcount) delete this; }
    
    void trigger(const T1 &t1, const T2 &t2, const T3 &t3) {
	if (_event_superbase::complete(true)) {
	    if (_t1) *_t1 = t1;
	    if (_t2) *_t2 = t2;
	    if (_t3) *_t3 = t3;
	}
    }

    void unbind() {
	_t1 = 0;
	_t2 = 0;
	_t3 = 0;
    }
    
  private:

    T1 *_t1;
    T2 *_t2;
    T3 *_t3;

    friend class event<T1, T2, T3>;
    
};



template <typename T1, typename T2>
class _event_base<T1, T2, void, void> : public _event_superbase { public:

    template <typename R, typename I1, typename I2>
    _event_base(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2)
	: _event_superbase(r, i1, i2), _t1(&t1), _t2(&t2) {
    }

    template <typename R, typename I1>
    _event_base(R &r, const I1 &i1, T1 &t1, T2 &t2)
	: _event_superbase(r, i1), _t1(&t1), _t2(&t2) {
    }

    template <typename R>
    _event_base(R &r, T1 &t1, T2 &t2)
	: _event_superbase(r), _t1(&t1), _t2(&t2) {
    }

    void unuse() { if (!--_refcount) delete this; }
    
    void trigger(const T1 &t1, const T2 &t2) {
	if (_event_superbase::complete(true)) {
	    if (_t1) *_t1 = t1;
	    if (_t2) *_t2 = t2;
	}
    }

    void unbind() {
	_t1 = 0;
	_t2 = 0;
    }
    
  private:

    T1 *_t1;
    T2 *_t2;

    friend class event<T1, T2>;
    
};


template <typename T1>
class _event_base<T1, void, void, void> : public _event_superbase { public:

    template <typename R, typename I1, typename I2>
    _event_base(R &r, const I1 &i1, const I2 &i2, T1 &t1)
	: _event_superbase(r, i1, i2), _t1(&t1) {
    }

    template <typename R, typename I1>
    _event_base(R &r, const I1 &i1, T1 &t1)
	: _event_superbase(r, i1), _t1(&t1) {
    }

    template <typename R>
    _event_base(R &r, T1 &t1)
	: _event_superbase(r), _t1(&t1) {
    }

    void unuse() { if (!--_refcount) delete this; }
    
    void trigger(const T1 &t1) {
	if (_event_superbase::complete(true)) {
	    if (_t1) *_t1 = t1;
	}
    }

    void unbind() {
	_t1 = 0;
    }
    
  private:

    T1 *_t1;

    _event_base(_rendezvous_superbase *r)
	: _event_superbase(r), _t1(0) {
    }
    
    friend class event<T1>;
    
};


template <typename T1, typename T2, typename T3, typename T4>
class event { public:

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _e(new _event_base<T1, T2, T3, T4>(r, i1, i2, t1, t2, t3, t4)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _e(new _event_base<T1, T2, T3, T4>(r, i1, t1, t2, t3, t4)) {
    }

    template <typename R>
    event(R &r, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _e(new _event_base<T1, T2, T3, T4>(r, t1, t2, t3, t4)) {
    }

    event(const event<T1, T2, T3, T4> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef _event_superbase::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
	_e->trigger(t1, t2, t3, t4);
    }

    void cancel() {
	_e->cancel();
    }

    template <typename R, typename I1, typename I2>
    event<T1, T2, T3, T4> make_rebind(R &r, const I1 &i1, const I2 &i2) {
	event<T1, T2, T3, T4> e(r, i1, i2, *_e->_t1, *_e->_t2, *_e->_t3, *_e->_t4);
	_e->unbind();
	return e;
    }

    template <typename R, typename I1>
    event<T1, T2, T3, T4> make_rebind(R &r, const I1 &i1) {
	event<T1, T2, T3, T4> e(r, i1, *_e->_t1, *_e->_t2, *_e->_t3, *_e->_t4);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T1, T2, T3, T4> make_rebind(R &r) {
	event<T1, T2, T3, T4> e(r, *_e->_t1, *_e->_t2, *_e->_t3, *_e->_t4);
	_e->unbind();
	return e;
    }
    
    event<T1, T2, T3, T4> &operator=(const event<T1, T2, T3, T4> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    _event_superbase *__superbase() const {
	return _e;
    }
    
  private:

    _event_base<T1, T2, T3, T4> *_e;
    
};


template <typename T1, typename T2, typename T3>
class event<T1, T2, T3, void> { public:

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3)
	: _e(new _event_base<T1, T2, T3, void>(r, i1, i2, t1, t2, t3)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3)
	: _e(new _event_base<T1, T2, T3, void>(r, i1, t1, t2, t3)) {
    }

    template <typename R>
    event(R &r, T1 &t1, T2 &t2, T3 &t3)
	: _e(new _event_base<T1, T2, T3, void>(r, t1, t2, t3)) {
    }

    event(const event<T1, T2, T3> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef _event_superbase::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger(const T1 &t1, const T2 &t2, const T3 &t3) {
	_e->trigger(t1, t2, t3);
    }

    template <typename R, typename I1, typename I2>
    event<T1, T2, T3> make_rebind(R &r, const I1 &i1, const I2 &i2) {
	event<T1, T2, T3> e(r, i1, i2, *_e->_t1, *_e->_t2, *_e->_t3);
	_e->unbind();
	return e;
    }

    template <typename R, typename I1>
    event<T1, T2, T3> make_rebind(R &r, const I1 &i1) {
	event<T1, T2, T3> e(r, i1, *_e->_t1, *_e->_t2, *_e->_t3);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T1, T2, T3> make_rebind(R &r) {
	event<T1, T2, T3> e(r, *_e->_t1, *_e->_t2, *_e->_t3);
	_e->unbind();
	return e;
    }

    event<T1, T2, T3> &operator=(const event<T1, T2, T3> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    _event_superbase *__superbase() const {
	return _e;
    }
    
  private:

    _event_base<T1, T2, T3, void> *_e;
    
};


template <typename T1, typename T2>
class event<T1, T2, void, void> { public:

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2)
	: _e(new _event_base<T1, T2, void, void>(r, i1, i2, t1, t2)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1, T2 &t2)
	: _e(new _event_base<T1, T2, void, void>(r, i1, t1, t2)) {
    }

    template <typename R>
    event(R &r, T1 &t1, T2 &t2)
	: _e(new _event_base<T1, T2, void, void>(r, t1, t2)) {
    }

    event(const event<T1, T2> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef _event_superbase::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger(const T1 &t1, const T2 &t2) {
	_e->trigger(t1, t2);
    }

    template <typename R, typename I1, typename I2>
    event<T1, T2> make_rebind(R &r, const I1 &i1, const I2 &i2) {
	event<T1, T2> e(r, i1, i2, *_e->_t1, *_e->_t2);
	_e->unbind();
	return e;
    }

    template <typename R, typename I1>
    event<T1, T2> make_rebind(R &r, const I1 &i1) {
	event<T1, T2> e(r, i1, *_e->_t1, *_e->_t2);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T1, T2> make_rebind(R &r) {
	event<T1, T2> e(r, *_e->_t1, *_e->_t2);
	_e->unbind();
	return e;
    }

    event<T1, T2> &operator=(const event<T1, T2> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    _event_superbase *__superbase() const {
	return _e;
    }
    
  private:

    _event_base<T1, T2, void, void> *_e;
    
};


template <typename T1>
class event<T1, void, void, void> { public:

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1)
	: _e(new _event_base<T1, void, void, void>(r, i1, i2, t1)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1)
	: _e(new _event_base<T1, void, void, void>(r, i1, t1)) {
    }

    template <typename R>
    event(R &r, T1 &t1)
	: _e(new _event_base<T1, void, void, void>(r, t1)) {
    }

    event(const event<T1> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef _event_superbase::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger(const T1 &t1) {
	_e->trigger(t1);
    }

    template <typename R, typename I1, typename I2>
    event<T1> make_rebind(R &r, const I1 &i1, const I2 &i2) {
	event<T1> e(r, i1, i2, *_e->_t1);
	_e->unbind();
	return e;
    }

    template <typename R, typename I1>
    event<T1> make_rebind(R &r, const I1 &i1) {
	event<T1> e(r, i1, *_e->_t1);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T1> make_rebind(R &r) {
	event<T1> e(r, *_e->_t1);
	_e->unbind();
	return e;
    }

    event<T1> &operator=(const event<T1> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    _event_superbase *__superbase() const {
	return _e;
    }
    
  private:

    _event_base<T1, void, void, void> *_e;

    event(_rendezvous_superbase *r)
	: _e(new _event_base<T1, void, void, void>(r)) {
    }
    
    friend class _unbind_rendezvous<T1>;
    
};


template <>
class event<void, void, void, void> { public:

    event() {
	if (!_event_superbase::dead)
	    _event_superbase::make_dead();
	_e = _event_superbase::dead;
	_e->use();
    }

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2)
	: _e(new _event_superbase(r, i1, i2)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1)
	: _e(new _event_superbase(r, i1)) {
    }

    template <typename R>
    event(R &r)
	: _e(new _event_superbase(r)) {
    }

    event(const event<> &e)
	: _e(e._e) {
	_e->use();
    }
    
    event(event<> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef _event_superbase::unspecified_bool_type unspecified_bool_type;
    
    operator unspecified_bool_type() const {
	return *_e;
    }

    bool operator!() const {
	return _e->empty();
    }
    
    bool empty() const {
	return _e->empty();
    }

    void at_cancel(const event<> &e) {
	_e->at_cancel(e);
    }

    void trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I1, typename I2>
    event<> make_rebind(R &r, const I1 &i1, const I2 &i2) {
	return event<>(r, i1, i2);
    }

    template <typename R, typename I1>
    event<> make_rebind(R &r, const I1 &i1) {
	return event<>(r, i1);
    }

    template <typename R>
    event<> make_rebind(R &r) {
	return event<>(r);
    }

    event<> &operator=(const event<> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }

    _event_superbase *__superbase() const {
	return _e;
    }

    static inline event<> __wrap(_event_superbase *e) {
	return event<>(marker(), e);
    }

  private:

    _event_superbase *_e;

    struct marker { };
    inline event(const marker &, _event_superbase *e)
	: _e(e) {
	_e->use();
    }
    
    friend class _event_superbase;
    friend event<> _hard_scatter(const event<> &, const event<> &);
    
};


inline void _event_superbase::at_cancel(const event<> &e)
{
    assert(_r);
    if (!_canceller) {
	_canceller = e._e;
	_canceller->use();
    } else {
	event<> comb = tamer::scatter(event<>::__wrap(_canceller), e);
	_canceller->unuse();
	_canceller = comb._e;
	_canceller->use();
    }
}


template <typename I1, typename I2, typename J1, typename J2, typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> make_event(rendezvous<I1, I2> &r, const J1 &i1, const J2 &i2, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
{
    return event<T1, T2, T3, T4>(r, i1, i2, t1, t2, t3, t4);
}

template <typename I1, typename J1, typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> make_event(rendezvous<I1> &r, const J1 &i1, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
{
    return event<T1, T2, T3, T4>(r, i1, t1, t2, t3, t4);
}

template <typename T1, typename T2, typename T3, typename T4>
inline event<T1, T2, T3, T4> make_event(rendezvous<> &r, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
{
    return event<T1, T2, T3, T4>(r, t1, t2, t3, t4);
}

template <typename I1, typename I2, typename J1, typename J2, typename T1, typename T2, typename T3>
inline event<T1, T2, T3> make_event(rendezvous<I1, I2> &r, const J1 &i1, const J2 &i2, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T1, T2, T3>(r, i1, i2, t1, t2, t3);
}

template <typename I1, typename J1, typename T1, typename T2, typename T3>
inline event<T1, T2, T3> make_event(rendezvous<I1> &r, const J1 &i1, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T1, T2, T3>(r, i1, t1, t2, t3);
}

template <typename T1, typename T2, typename T3>
inline event<T1, T2, T3> make_event(rendezvous<> &r, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T1, T2, T3>(r, t1, t2, t3);
}

template <typename I1, typename I2, typename J1, typename J2, typename T1, typename T2>
inline event<T1, T2> make_event(rendezvous<I1, I2> &r, const J1 &i1, const J2 &i2, T1 &t1, T2 &t2)
{
    return event<T1, T2>(r, i1, i2, t1, t2);
}

template <typename I1, typename J1, typename T1, typename T2>
inline event<T1, T2> make_event(rendezvous<I1> &r, const J1 &i1, T1 &t1, T2 &t2)
{
    return event<T1, T2>(r, i1, t1, t2);
}

template <typename T1, typename T2>
inline event<T1, T2> make_event(rendezvous<> &r, T1 &t1, T2 &t2)
{
    return event<T1, T2>(r, t1, t2);
}

template <typename I1, typename I2, typename J1, typename J2, typename T1>
inline event<T1> make_event(rendezvous<I1, I2> &r, const J1 &i1, const J2 &i2, T1 &t1)
{
    return event<T1>(r, i1, i2, t1);
}

template <typename I1, typename J1, typename T1>
inline event<T1> make_event(rendezvous<I1> &r, const J1 &i1, T1 &t1)
{
    return event<T1>(r, i1, t1);
}

template <typename T1>
inline event<T1> make_event(rendezvous<> &r, T1 &t1)
{
    return event<T1>(r, t1);
}

template <typename I1, typename I2, typename J1, typename J2>
inline event<> make_event(rendezvous<I1, I2> &r, const J1 &i1, const J2 &i2)
{
    return event<>(r, i1, i2);
}

template <typename I1, typename J1>
inline event<> make_event(rendezvous<I1> &r, const J1 &i1)
{
    return event<>(r, i1);
}

inline event<> make_event(rendezvous<> &r)
{
    return event<>(r);
}

inline event<> scatter(const event<> &e1, const event<> &e2) {
    if (e1.empty())
	return e2;
    else if (e2.empty())
	return e1;
    else
	return _hard_scatter(e1, e2);
}

}
#endif /* TAMER__EVENT_HH */
