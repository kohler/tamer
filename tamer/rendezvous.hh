#ifndef TAMER__RENDEZVOUS_HH
#define TAMER__RENDEZVOUS_HH
#include <tamer/_util.hh>
#include <tamer/_base.hh>
namespace tamer {

template <typename I1, typename I2>
class rendezvous : public _rendezvous_base { public:

    rendezvous();

    void add(_event_superbase *e, const I1 &i1, const I2 &i2);
    void complete(uintptr_t rname, bool success);
    bool join(I1 &, I2 &);

    unsigned nready() const	{ return _bs.nactive(); }
    unsigned nwaiting() const	{ return _bs.npassive(); }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  private:

    struct evtrec {
	size_t next;
	I1 i1;
	I2 i2;
    };

    blockset<evtrec> _bs;
    
};

template <typename I1, typename I2>
rendezvous<I1, I2>::rendezvous()
{
}

template <typename I1, typename I2>
void rendezvous<I1, I2>::add(_event_superbase *e, const I1 &i1, const I2 &i2)
{
    uintptr_t u;
    evtrec &ew = _bs.allocate_passive(u);
    (void) new(static_cast<void *>(&ew.i1)) I1(i1);
    (void) new(static_cast<void *>(&ew.i2)) I2(i2);
    e->initialize(this, u);
}

template <typename I1, typename I2>
void rendezvous<I1, I2>::complete(uintptr_t rname, bool success)
{
    if (success) {
	_bs.activate(rname);
	_unblock();
    } else
	_bs.kill(rname);
}

template <typename I1, typename I2>
bool rendezvous<I1, I2>::join(I1 &i1, I2 &i2)
{
    if (evtrec *e = _bs.active_front()) {
	i1 = e->i1;
	i2 = e->i2;
	_bs.pop_active();
	return true;
    } else
	return false;
}


template <typename I1>
class rendezvous<I1, void> : public _rendezvous_base { public:

    rendezvous();

    void add(_event_superbase *e, const I1 &i1);
    void complete(uintptr_t rname, bool success);
    bool join(I1 &);

    unsigned nready() const	{ return _bs.nactive(); }
    unsigned nwaiting() const	{ return _bs.npassive(); }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  private:

    struct evtrec {
	size_t next;
	I1 i1;
    };

    blockset<evtrec> _bs;
    
};

template <typename I1>
rendezvous<I1, void>::rendezvous()
{
}

template <typename I1>
void rendezvous<I1, void>::add(_event_superbase *e, const I1 &i1)
{
    uintptr_t u;
    evtrec &erec = _bs.allocate_passive(u);
    (void) new(static_cast<void *>(&erec.i1)) I1(i1);
    e->initialize(this, u);
}

template <typename I1>
void rendezvous<I1, void>::complete(uintptr_t rname, bool success)
{
    if (success) {
	_bs.activate(rname);
	_unblock();
    } else
	_bs.cancel(rname);
}

template <typename I1>
bool rendezvous<I1, void>::join(I1 &i1)
{
    if (evtrec *e = _bs.active_front()) {
	i1 = e->i1;
	_bs.pop_active();
	return true;
    } else
	return false;
}


template <>
class rendezvous<uintptr_t> : public _rendezvous_base { public:

    inline rendezvous();

    inline void add(_event_superbase *e, uintptr_t i1) throw ();
    inline void complete(uintptr_t rname, bool success);
    inline bool join(uintptr_t &);

    unsigned nready() const	{ return _buf.size(); }
    unsigned nwaiting() const	{ return _nwaiting; }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  private:

    debuffer<uintptr_t> _buf;
    size_t _nwaiting;
    
};

inline rendezvous<uintptr_t>::rendezvous()
    : _nwaiting(0)
{
}

inline void rendezvous<uintptr_t>::add(_event_superbase *e, uintptr_t i1) throw ()
{
    _nwaiting++;
    e->initialize(this, i1);
}

inline void rendezvous<uintptr_t>::complete(uintptr_t rname, bool success)
{
    if (success) {
	_buf.push_back(rname);
	_unblock();
    }
    _nwaiting--;
}

inline bool rendezvous<uintptr_t>::join(uintptr_t &i1)
{
    if (uintptr_t *x = _buf.front()) {
	i1 = *x;
	_buf.pop_front();
	return true;
    } else
	return false;
}



template <typename T>
class rendezvous<T *> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add(_event_superbase *e, T *i1) throw () {
	inherited::add(e, reinterpret_cast<uintptr_t>(i1));
    }
    
    inline bool join(T *&i1) {
	uintptr_t u;
	if (inherited::join(u)) {
	    i1 = reinterpret_cast<T *>(u);
	    return true;
	} else
	    return false;
    }
        
};


template <>
class rendezvous<int> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add(_event_superbase *e, int i1) throw () {
	inherited::add(e, static_cast<uintptr_t>(i1));
    }
    
    inline bool join(int &i1) {
	uintptr_t u;
	if (inherited::join(u)) {
	    i1 = static_cast<int>(u);
	    return true;
	} else
	    return false;
    }

};


template <>
class rendezvous<bool> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add(_event_superbase *e, bool i1) throw () {
	inherited::add(e, static_cast<uintptr_t>(i1));
    }
    
    inline bool join(bool &i1) {
	uintptr_t u;
	if (inherited::join(u)) {
	    i1 = static_cast<bool>(u);
	    return true;
	} else
	    return false;
    }

};


template <>
class rendezvous<void> : public _rendezvous_base { public:

    rendezvous() : _nwaiting(0), _nready(0) { }

    inline void add(_event_superbase *e) throw () {
	_nwaiting++;
	e->initialize(this, 1);
    }
    
    inline void complete(uintptr_t, bool success) {
	_nwaiting--;
	if (success) {
	    _nready++;
	    _unblock();
	}
    }
    
    inline bool join() {
	if (_nready) {
	    _nready--;
	    return true;
	} else
	    return false;
    }

    unsigned nready() const	{ return _nready; }
    unsigned nwaiting() const	{ return _nwaiting; }
    unsigned nevents() const	{ return nready() + nwaiting(); }

  protected:

    unsigned _nwaiting;
    unsigned _nready;
    
};


class gather_rendezvous : public rendezvous<> { public:

    gather_rendezvous() {
    }

    inline void complete(uintptr_t, bool success) {
	_nwaiting--;
	if (success)
	    _nready++;
	if (_nwaiting == 0)
	    _unblock();
    }

};

}
#endif /* TAMER__RENDEZVOUS_HH */
