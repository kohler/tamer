#ifndef TAMER_XEVENT_HH
#define TAMER_XEVENT_HH 1
#include <tamer/rendezvous.hh>
namespace tamer {
namespace tamerpriv {

template <typename R, typename I0, typename I1>
inline simple_event::simple_event(R &r, const I0 &i0, const I1 &i1, void *s0)
    : _refcount(1), _weak_refcount(0), _canceler(0), _s0(s0)
{
    r.add(this, i0, i1);
}

template <typename R, typename I0>
inline simple_event::simple_event(R &r, const I0 &i0, void *s0)
    : _refcount(1), _weak_refcount(0), _canceler(0), _s0(s0)
{
    r.add(this, i0);
}

template <typename R>
inline simple_event::simple_event(R &r, void *s0)
    : _refcount(1), _weak_refcount(0), _canceler(0), _s0(s0)
{
    r.add(this);
}


template <typename T0, typename T1, typename T2, typename T3>
class eventx : public simple_event { public:

    eventx()
	: simple_event(), _s1(0), _s2(0), _s3(0) {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 *s0, T1 *s1, T2 *s2, T3 *s3)
	: simple_event(r, i0, i1, s0), _s1(s1), _s2(s2), _s3(s3) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 *s0, T1 *s1, T2 *s2, T3 *s3)
	: simple_event(r, i0, s0), _s1(s1), _s2(s2), _s3(s3) {
    }

    template <typename R>
    eventx(R &r, T0 *s0, T1 *s1, T2 *s2, T3 *s3)
	: simple_event(r, s0), _s1(s1), _s2(s2), _s3(s3) {
    }

    void unuse() {
	if (__unuse())
	    delete this;
    }
    
    void trigger(const T0 &v0, const T1 &v1, const T2 &v2, const T3 &v3) {
	if (simple_event::complete(true)) {
	    if (_s0) *static_cast<T0 *>(_s0) = v0;
	    if (_s1) *_s1 = v1;
	    if (_s2) *_s2 = v2;
	    if (_s3) *_s3 = v3;
	}
    }

    void unbind() {
	_s0 = 0;
	_s1 = 0;
	_s2 = 0;
	_s3 = 0;
    }
    
  private:

    T1 *_s1;
    T2 *_s2;
    T3 *_s3;

    friend class event<T0, T1, T2, T3>;
    
};


template <typename T0, typename T1, typename T2>
class eventx<T0, T1, T2, void> : public simple_event { public:

    eventx()
	: simple_event(), _s1(0), _s2(0) {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 *s0, T1 *s1, T2 *s2)
	: simple_event(r, i0, i1, s0), _s1(s1), _s2(s2) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 *s0, T1 *s1, T2 *s2)
	: simple_event(r, i0, s0), _s1(s1), _s2(s2) {
    }

    template <typename R>
    eventx(R &r, T0 *s0, T1 *s1, T2 *s2)
	: simple_event(r, s0), _s1(s1), _s2(s2) {
    }

    void unuse() {
	if (__unuse())
	    delete this;
    }
    
    void trigger(const T0 &v0, const T1 &v1, const T2 &v2) {
	if (simple_event::complete(true)) {
	    if (_s0) *static_cast<T0 *>(_s0) = v0;
	    if (_s1) *_s1 = v1;
	    if (_s2) *_s2 = v2;
	}
    }

    void unbind() {
	_s0 = 0;
	_s1 = 0;
	_s2 = 0;
    }
    
  private:

    T1 *_s1;
    T2 *_s2;

    friend class event<T0, T1, T2>;
    
};



template <typename T0, typename T1>
class eventx<T0, T1, void, void> : public simple_event { public:

    eventx()
	: simple_event(), _s1(0) {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 *s0, T1 *s1)
	: simple_event(r, i0, i1, s0), _s1(s1) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 *s0, T1 *s1)
	: simple_event(r, i0, s0), _s1(s1) {
    }

    template <typename R>
    eventx(R &r, T0 *s0, T1 *s1)
	: simple_event(r, s0), _s1(s1) {
    }

    void unuse() {
	if (__unuse())
	    delete this;
    }
    
    void trigger(const T0 &v0, const T1 &v1) {
	if (simple_event::complete(true)) {
	    if (_s0) *static_cast<T0 *>(_s0) = v0;
	    if (_s1) *_s1 = v1;
	}
    }

    void unbind() {
	_s0 = 0;
	_s1 = 0;
    }
    
  private:

    T1 *_s1;

    friend class event<T0, T1>;
    
};


template <typename T0>
class eventx<T0, void, void, void> : public simple_event { public:

    eventx()
	: simple_event() {
    }
    
    template <typename R, typename I0, typename I1>
    eventx(R &r, const I0 &i0, const I1 &i1, T0 *s0)
	: simple_event(r, i0, i1, s0) {
    }

    template <typename R, typename I0>
    eventx(R &r, const I0 &i0, T0 *s0)
	: simple_event(r, i0, s0) {
    }

    template <typename R>
    eventx(R &r, T0 *s0)
	: simple_event(r, s0) {
    }

    void unuse() {
	if (__unuse())
	    delete this;
    }
    
    void trigger(const T0 &v0) {
	if (simple_event::complete(true)) {
	    if (_s0) *static_cast<T0 *>(_s0) = v0;
	}
    }

    void unbind() {
	_s0 = 0;
    }

    T0 *slot0() const {
	return static_cast<T0 *>(_s0);
    }
    
  private:

    friend class event<T0>;
    
};

}}
#endif /* TAMER_XEVENT_HH */
