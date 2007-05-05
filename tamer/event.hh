#ifndef TAMER_EVENT_HH
#define TAMER_EVENT_HH 1
#include <tamer/rendezvous.hh>
namespace tamer {
namespace tamerpriv {

template <typename R, typename I1, typename I2>
simple_event::simple_event(R &r, const I1 &i1, const I2 &i2)
    : _refcount(1), _canceller(0)
{
    r.add(this, i1, i2);
}

template <typename R, typename I1>
simple_event::simple_event(R &r, const I1 &i1)
    : _refcount(1), _canceller(0)
{
    r.add(this, i1);
}

template <typename R>
inline simple_event::simple_event(R &r)
    : _refcount(1), _canceller(0)
{
    r.add(this);
}


template <typename T1, typename T2, typename T3, typename T4>
class eventx : public simple_event { public:

    eventx()
	: simple_event(), _t1(0), _t2(0), _t3(0), _t4(0) {
    }
    
    template <typename R, typename I1, typename I2>
    eventx(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: simple_event(r, i1, i2), _t1(&t1), _t2(&t2), _t3(&t3), _t4(&t4) {
    }

    template <typename R, typename I1>
    eventx(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: simple_event(r, i1), _t1(&t1), _t2(&t2), _t3(&t3), _t4(&t4) {
    }

    template <typename R>
    eventx(R &r, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: simple_event(r), _t1(&t1), _t2(&t2), _t3(&t3), _t4(&t4) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
	if (simple_event::complete(true)) {
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
class eventx<T1, T2, T3, void> : public simple_event { public:

    eventx()
	: simple_event(), _t1(0), _t2(0), _t3(0) {
    }
    
    template <typename R, typename I1, typename I2>
    eventx(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3)
	: simple_event(r, i1, i2), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    template <typename R, typename I1>
    eventx(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3)
	: simple_event(r, i1), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    template <typename R>
    eventx(R &r, T1 &t1, T2 &t2, T3 &t3)
	: simple_event(r), _t1(&t1), _t2(&t2), _t3(&t3) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T1 &t1, const T2 &t2, const T3 &t3) {
	if (simple_event::complete(true)) {
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
class eventx<T1, T2, void, void> : public simple_event { public:

    eventx()
	: simple_event(), _t1(0), _t2(0) {
    }
    
    template <typename R, typename I1, typename I2>
    eventx(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2)
	: simple_event(r, i1, i2), _t1(&t1), _t2(&t2) {
    }

    template <typename R, typename I1>
    eventx(R &r, const I1 &i1, T1 &t1, T2 &t2)
	: simple_event(r, i1), _t1(&t1), _t2(&t2) {
    }

    template <typename R>
    eventx(R &r, T1 &t1, T2 &t2)
	: simple_event(r), _t1(&t1), _t2(&t2) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T1 &t1, const T2 &t2) {
	if (simple_event::complete(true)) {
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
class eventx<T1, void, void, void> : public simple_event { public:

    eventx()
	: simple_event(), _t1(0) {
    }
    
    template <typename R, typename I1, typename I2>
    eventx(R &r, const I1 &i1, const I2 &i2, T1 &t1)
	: simple_event(r, i1, i2), _t1(&t1) {
    }

    template <typename R, typename I1>
    eventx(R &r, const I1 &i1, T1 &t1)
	: simple_event(r, i1), _t1(&t1) {
    }

    template <typename R>
    eventx(R &r, T1 &t1)
	: simple_event(r), _t1(&t1) {
    }

    void unuse() {
	if (!--_refcount)
	    delete this;
    }
    
    void trigger(const T1 &t1) {
	if (simple_event::complete(true)) {
	    if (_t1) *_t1 = t1;
	}
    }

    void unbind() {
	_t1 = 0;
    }

    T1 *slot1() const {
	return _t1;
    }
    
  private:

    T1 *_t1;
    
    friend class event<T1>;
    
};

} /* namespace tamerpriv */


template <typename T1, typename T2, typename T3, typename T4>
class event { public:

    event()
	: _e(new tamerpriv::eventx<T1, T2, T3, T4>()) {
    }
    
    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _e(new tamerpriv::eventx<T1, T2, T3, T4>(r, i1, i2, t1, t2, t3, t4)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _e(new tamerpriv::eventx<T1, T2, T3, T4>(r, i1, t1, t2, t3, t4)) {
    }

    template <typename R>
    event(R &r, T1 &t1, T2 &t2, T3 &t3, T4 &t4)
	: _e(new tamerpriv::eventx<T1, T2, T3, T4>(r, t1, t2, t3, t4)) {
    }

    event(const event<T1, T2, T3, T4> &e)
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

    void trigger(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
	_e->trigger(t1, t2, t3, t4);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
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
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T1, T2, T3, T4> *_e;
    
};


template <typename T1, typename T2, typename T3>
class event<T1, T2, T3, void> { public:

    event()
	: _e(new tamerpriv::eventx<T1, T2, T3, void>()) {
    }

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T1, T2, T3, void>(r, i1, i2, t1, t2, t3)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T1, T2, T3, void>(r, i1, t1, t2, t3)) {
    }

    template <typename R>
    event(R &r, T1 &t1, T2 &t2, T3 &t3)
	: _e(new tamerpriv::eventx<T1, T2, T3, void>(r, t1, t2, t3)) {
    }

    event(const event<T1, T2, T3> &e)
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

    void trigger(const T1 &t1, const T2 &t2, const T3 &t3) {
	_e->trigger(t1, t2, t3);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
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
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T1, T2, T3, void> *_e;
    
};


template <typename T1, typename T2>
class event<T1, T2, void, void> { public:

    event()
	: _e(new tamerpriv::eventx<T1, T2, void, void>()) {
    }

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T1, T2, void, void>(r, i1, i2, t1, t2)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T1, T2, void, void>(r, i1, t1, t2)) {
    }

    template <typename R>
    event(R &r, T1 &t1, T2 &t2)
	: _e(new tamerpriv::eventx<T1, T2, void, void>(r, t1, t2)) {
    }

    event(const event<T1, T2> &e)
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

    void trigger(const T1 &t1, const T2 &t2) {
	_e->trigger(t1, t2);
    }

    void unbound_trigger() {
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
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
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T1, T2, void, void> *_e;
    
};


template <typename T1>
class event<T1, void, void, void> { public:

    event()
	: _e(new tamerpriv::eventx<T1, void, void, void>()) {
    }

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2, T1 &t1)
	: _e(new tamerpriv::eventx<T1, void, void, void>(r, i1, i2, t1)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1, T1 &t1)
	: _e(new tamerpriv::eventx<T1, void, void, void>(r, i1, t1)) {
    }

    template <typename R>
    event(R &r, T1 &t1)
	: _e(new tamerpriv::eventx<T1, void, void, void>(r, t1)) {
    }

    event(const event<T1> &e)
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

    void trigger(const T1 &t1) {
	_e->trigger(t1);
    }

    void unbound_trigger() {
	_e->unbind();
	_e->complete(true);
    }

    void cancel() {
	_e->complete(false);
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

    T1 *slot1() const {
	return _e->slot1();
    }
    
    event<T1> &operator=(const event<T1> &e) {
	e._e->use();
	_e->unuse();
	_e = e._e;
	return *this;
    }
    
    tamerpriv::simple_event *__get_simple() const {
	return _e;
    }
    
  private:

    tamerpriv::eventx<T1, void, void, void> *_e;
    
};


template <>
class event<void, void, void, void> { public:

    event() {
	if (!tamerpriv::simple_event::dead)
	    tamerpriv::simple_event::make_dead();
	_e = tamerpriv::simple_event::dead;
	_e->use();
    }

    template <typename R, typename I1, typename I2>
    event(R &r, const I1 &i1, const I2 &i2)
	: _e(new tamerpriv::simple_event(r, i1, i2)) {
    }

    template <typename R, typename I1>
    event(R &r, const I1 &i1)
	: _e(new tamerpriv::simple_event(r, i1)) {
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
    else if (!_canceller) {
	_canceller = e._e;
	_canceller->use();
    } else {
	event<> comb = tamer::distribute(event<>::__take(_canceller), e);
	_canceller = comb._e;
	_canceller->use();
    }
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
