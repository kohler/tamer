#ifndef TAMER_EVENTX_HH
#define TAMER_EVENTX_HH 1
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

    T0 *slot0() const {
	return _t0;
    }
    
  private:

    T0 *_t0;
    
    friend class event<T0>;
    
};

}}
#endif /* TAMER_EVENTX_HH */
