#ifndef TAMER_EVENT_HH
#define TAMER_EVENT_HH 1
#include <tamer/rendezvous.hh>
namespace tamer {
namespace tamerpriv {

template <typename R, typename I0, typename I1>
simple_event::simple_event(R &r, const I0 &i0, const I1 &i1)
    : _refcount(1), _canceler(0)
{
    r.add(this, i0, i1);
}

template <typename R, typename I0>
simple_event::simple_event(R &r, const I0 &i0)
    : _refcount(1), _canceler(0)
{
    r.add(this, i0);
}

template <typename R>
inline simple_event::simple_event(R &r)
    : _refcount(1), _canceler(0)
{
    r.add(this);
}


template <typename T0, typename T1, typename T2, typename T3>
class eventx : public simple_event { public:

    eventx()
	: simple_event(), _t0(0), _t1(0), _t2(0), _t3(0) {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: simple_event(r, i0, i1), _t0(&t0), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: simple_event(r, i0), _t0(&t0), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    template <typename R>
    eventx(R &r, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: simple_event(r), _t0(&t0), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T0 &v0, const T1 &v1, const T2 &v2, const T3 &v3) {
	if (simple_event::complete(true)) {
	    if (_t0) *_t0 = v0;
	    if (_t1) *_t1 = v1;
	    if (_t2) *_t2 = v2;
	    if (_t3) *_t3 = v3;
	}
    }

    void unbind() {
	_t0 = 0;
	_t1 = 0;
	_t2 = 0;
	_t3 = 0;
    }
    
  private:

    T0 *_t0;
    T1 *_t1;
    T2 *_t2;
    T3 *_t3;

    friend class event<T0, T1, T2, T3>;
    
};


template <typename T0, typename T1, typename T2>
class eventx<T0, T1, T2, void> : public simple_event { public:

    eventx()
	: simple_event(), _t0(0), _t1(0), _t2(0) {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1, T2 &t2)
	: simple_event(r, i0, i1), _t0(&t0), _t1(&t1), _t2(&t2) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 &t0, T1 &t1, T2 &t2)
	: simple_event(r, i0), _t0(&t0), _t1(&t1), _t2(&t2) {
    }

    template <typename R>
    eventx(R &r, T0 &t0, T1 &t1, T2 &t2)
	: simple_event(r), _t0(&t0), _t1(&t1), _t2(&t2) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T0 &v0, const T1 &v1, const T2 &v2) {
	if (simple_event::complete(true)) {
	    if (_t0) *_t0 = v0;
	    if (_t1) *_t1 = v1;
	    if (_t2) *_t2 = v2;
	}
    }

    void unbind() {
	_t0 = 0;
	_t1 = 0;
	_t2 = 0;
    }
    
  private:

    T0 *_t0;
    T1 *_t1;
    T2 *_t2;

    friend class event<T0, T1, T2>;
    
};



template <typename T0, typename T1>
class eventx<T0, T1, void, void> : public simple_event { public:

    eventx()
	: simple_event(), _t0(0), _t1(0) {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1)
	: simple_event(r, i0, i1), _t0(&t0), _t1(&t1) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 &t0, T1 &t1)
	: simple_event(r, i0), _t0(&t0), _t1(&t1) {
    }

    template <typename R>
    eventx(R &r, T0 &t0, T1 &t1)
	: simple_event(r), _t0(&t0), _t1(&t1) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T0 &v0, const T1 &v1) {
	if (simple_event::complete(true)) {
	    if (_t0) *_t0 = v0;
	    if (_t1) *_t1 = v1;
	}
    }

    void unbind() {
	_t0 = 0;
	_t1 = 0;
    }
    
  private:

    T0 *_t0;
    T1 *_t1;

    friend class event<T0, T1>;
    
};


template <typename T0>
class eventx<T0, void, void, void> : public simple_event { public:

    eventx()
	: simple_event(), _t0(0) {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 &t0)
	: simple_event(r, i0, i1), _t0(&t0) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 &t0)
	: simple_event(r, i0), _t0(&t0) {
    }

    template <typename R>
    eventx(R &r, T0 &t0)
	: simple_event(r), _t0(&t0) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T0 &v0) {
	if (simple_event::complete(true)) {
	    if (_t0) *_t0 = v0;
	}
    }

    void unbind() {
	_t0 = 0;
    }

    T0 *slot(int i) const {
	assert(i == 0);
	return _t0;
    }
    
  private:

    T0 *_t0;
    
    friend class event<T0>;
    
};

} /* namespace tamerpriv */


template <typename T0, typename T1, typename T2, typename T3>
class event { public:

    event()
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>()) {
    }
    
    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, i0, i1, t0, t1, t2, t3)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, i0, t0, t1, t2, t3)) {
    }

    template <typename R>
    event(R &r, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T0, T1, T2, T3>(r, t0, t1, t2, t3)) {
    }

    event(const event<T0, T1, T2, T3> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
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

    void trigger(const T0 &v0, const T1 &v1, const T2 &v2, const T3 &v3) {
	_e->trigger(v0, v1, v2, v3);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<T0, T1, T2, T3> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	event<T0, T1, T2, T3> e(r, i0, i1, *_e->_t0, *_e->_t1, *_e->_t2, *_e->_t3);
	_e->unbind();
	return e;
    }

    template <typename R, typename I0>
    event<T0, T1, T2, T3> make_rebind(R &r, const I0 &i0) {
	event<T0, T1, T2, T3> e(r, i0, *_e->_t0, *_e->_t1, *_e->_t2, *_e->_t3);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T0, T1, T2, T3> make_rebind(R &r) {
	event<T0, T1, T2, T3> e(r, *_e->_t0, *_e->_t1, *_e->_t2, *_e->_t3);
	_e->unbind();
	return e;
    }
    
    event<T0, T1, T2, T3> &operator=(const event<T0, T1, T2, T3> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, T1, T2, T3> *_e;
    
};


template <typename T0, typename T1, typename T2>
class event<T0, T1, T2, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, T1, T2, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, i0, i1, t0, t1, t2)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, i0, t0, t1, t2)) {
    }

    template <typename R>
    event(R &r, T0 &t0, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T0, T1, T2, void>(r, t0, t1, t2)) {
    }

    event(const event<T0, T1, T2> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
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

    void trigger(const T0 &v0, const T1 &v1, const T2 &v2) {
	_e->trigger(v0, v1, v2);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<T0, T1, T2> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	event<T0, T1, T2> e(r, i0, i1, *_e->_t0, *_e->_t1, *_e->_t2);
	_e->unbind();
	return e;
    }

    template <typename R, typename I0>
    event<T0, T1, T2> make_rebind(R &r, const I0 &i0) {
	event<T0, T1, T2> e(r, i0, *_e->_t0, *_e->_t1, *_e->_t2);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T0, T1, T2> make_rebind(R &r) {
	event<T0, T1, T2> e(r, *_e->_t0, *_e->_t1, *_e->_t2);
	_e->unbind();
	return e;
    }

    event<T0, T1, T2> &operator=(const event<T0, T1, T2> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, T1, T2, void> *_e;
    
};


template <typename T0, typename T1>
class event<T0, T1, void, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, T1, void, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0, T1 &t1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, i0, i1, t0, t1)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0, T1 &t1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, i0, t0, t1)) {
    }

    template <typename R>
    event(R &r, T0 &t0, T1 &t1)
	: _e(new tamerpriv::eventx<T0, T1, void, void>(r, t0, t1)) {
    }

    event(const event<T0, T1> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
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

    void trigger(const T0 &v0, const T1 &v1) {
	_e->trigger(v0, v1);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<T0, T1> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	event<T0, T1> e(r, i0, i1, *_e->_t0, *_e->_t1);
	_e->unbind();
	return e;
    }

    template <typename R, typename I0>
    event<T0, T1> make_rebind(R &r, const I0 &i0) {
	event<T0, T1> e(r, i0, *_e->_t0, *_e->_t1);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T0, T1> make_rebind(R &r) {
	event<T0, T1> e(r, *_e->_t0, *_e->_t1);
	_e->unbind();
	return e;
    }

    event<T0, T1> &operator=(const event<T0, T1> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, T1, void, void> *_e;
    
};


template <typename T0>
class event<T0, void, void, void> { public:

    event()
	: _e(new tamerpriv::eventx<T0, void, void, void>()) {
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1, T0 &t0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, i1, t0)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0, T0 &t0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, i0, t0)) {
    }

    template <typename R>
    event(R &r, T0 &t0)
	: _e(new tamerpriv::eventx<T0, void, void, void>(r, t0)) {
    }

    event(const event<T0> &e)
	: _e(e._e) {
	_e->use();
    }
    
    ~event() {
	_e->unuse();
    }

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
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

    void trigger(const T0 &v0) {
	_e->trigger(v0);
    }

    void unbound_trigger() {
	_e->unbind();
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<T0> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	event<T0> e(r, i0, i1, *_e->_t0);
	_e->unbind();
	return e;
    }

    template <typename R, typename I0>
    event<T0> make_rebind(R &r, const I0 &i0) {
	event<T0> e(r, i0, *_e->_t0);
	_e->unbind();
	return e;
    }

    template <typename R>
    event<T0> make_rebind(R &r) {
	event<T0> e(r, *_e->_t0);
	_e->unbind();
	return e;
    }

    T0 *slot0() const {
	return _e->slot(0);
    }
    
    event<T0> &operator=(const event<T0> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T0, void, void, void> *_e;
    
};


template <>
class event<void, void, void, void> { public:

    event() {
	if (!tamerpriv::simple_event::dead)
	    tamerpriv::simple_event::make_dead();
	_e = tamerpriv::simple_event::dead;
	_e->use();
    }

    template <typename R, typename I0, typename I1>
    event(R &r, const I0 &i0, const I1 &i1)
	: _e(new tamerpriv::simple_event(r, i0, i1)) {
    }

    template <typename R, typename I0>
    event(R &r, const I0 &i0)
	: _e(new tamerpriv::simple_event(r, i0)) {
    }

    template <typename R>
    explicit event(R &r)
	: _e(new tamerpriv::simple_event(r)) {
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

    typedef tamerpriv::simple_event::unspecified_bool_type unspecified_bool_type;
    
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

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
    }

    template <typename R, typename I0, typename I1>
    event<> make_rebind(R &r, const I0 &i0, const I1 &i1) {
	return event<>(r, i0, i1);
    }

    template <typename R, typename I0>
    event<> make_rebind(R &r, const I0 &i0) {
	return event<>(r, i0);
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

    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }

    static inline event<> __take(tamerpriv::simple_event *e) {
	return event<>(take_marker(), e);
    }

  private:

    tamerpriv::simple_event *_e;

    struct take_marker { };
    inline event(const take_marker &, tamerpriv::simple_event *e)
	: _e(e) {
    }
    
    friend class tamerpriv::simple_event;
    
};


namespace tamerpriv {
inline void simple_event::at_cancel(const event<> &e)
{
    if (!_r)
	e._e->complete(true);
    else if (!_canceler) {
	_canceler = e._e;
	_canceler->use();
    } else {
	event<> comb = tamer::distribute(event<>::__take(_canceler), e);
	_canceler = comb._e;
	_canceler->use();
    }
}
}


template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T0, T1, T2, T3>(r, i0, i1, t0, t1, t2, t3);
}

template <typename I0, typename J0, typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T0, T1, T2, T3>(r, i0, t0, t1, t2, t3);
}

template <typename T0, typename T1, typename T2, typename T3>
inline event<T0, T1, T2, T3> make_event(rendezvous<> &r, T0 &t0, T1 &t1, T2 &t2, T3 &t3)
{
    return event<T0, T1, T2, T3>(r, t0, t1, t2, t3);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0, T1 &t1, T2 &t2)
{
    return event<T0, T1, T2>(r, i0, i1, t0, t1, t2);
}

template <typename I0, typename J0, typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0, T1 &t1, T2 &t2)
{
    return event<T0, T1, T2>(r, i0, t0, t1, t2);
}

template <typename T0, typename T1, typename T2>
inline event<T0, T1, T2> make_event(rendezvous<> &r, T0 &t0, T1 &t1, T2 &t2)
{
    return event<T0, T1, T2>(r, t0, t1, t2);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0, T1 &t1)
{
    return event<T0, T1>(r, i0, i1, t0, t1);
}

template <typename I0, typename J0, typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0, T1 &t1)
{
    return event<T0, T1>(r, i0, t0, t1);
}

template <typename T0, typename T1>
inline event<T0, T1> make_event(rendezvous<> &r, T0 &t0, T1 &t1)
{
    return event<T0, T1>(r, t0, t1);
}

template <typename I0, typename I1, typename J0, typename J1, typename T0>
inline event<T0> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1, T0 &t0)
{
    return event<T0>(r, i0, i1, t0);
}

template <typename I0, typename J0, typename T0>
inline event<T0> make_event(rendezvous<I0> &r, const J0 &i0, T0 &t0)
{
    return event<T0>(r, i0, t0);
}

template <typename T0>
inline event<T0> make_event(rendezvous<> &r, T0 &t0)
{
    return event<T0>(r, t0);
}

template <typename I0, typename I1, typename J0, typename J1>
inline event<> make_event(rendezvous<I0, I1> &r, const J0 &i0, const J1 &i1)
{
    return event<>(r, i0, i1);
}

template <typename I0, typename J0>
inline event<> make_event(rendezvous<I0> &r, const J0 &i0)
{
    return event<>(r, i0);
}

inline event<> make_event(rendezvous<> &r)
{
    return event<>(r);
}

inline event<> distribute(const event<> &e1, const event<> &e2) {
    if (e1.empty())
	return e2;
    else if (e2.empty())
	return e1;
    else
	return tamerpriv::hard_distribute(e1, e2);
}

inline event<> distribute(const event<> &e1, const event<> &e2, const event<> &e3) {
    return distribute(distribute(e1, e2), e3);
}

}
#endif /* TAMER__EVENT_HH */
