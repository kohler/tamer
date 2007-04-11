#ifndef TAMER_TAME_RENDEZVOUS_HH
#define TAMER_TAME_RENDEZVOUS_HH
#include "tame_util.hh"
#include "tame_base.hh"
namespace tame {

template <typename W1, typename W2>
class rendezvous : public _rendezvous_base { public:

    rendezvous();

    void add_event(_event_superbase *e, const W1 &w1, const W2 &w2);
    void _complete(uintptr_t rname, bool success);
    bool join(W1 &, W2 &);

    unsigned nactive() const	{ return _bs.nactive(); }
    unsigned npassive() const	{ return _bs.npassive(); }
    unsigned nevents() const	{ return nactive() + npassive(); }

  private:

    struct evtrec {
	size_t next;
	W1 w1;
	W2 w2;
    };

    blockset<evtrec> _bs;
    
};

template <typename W1, typename W2>
rendezvous<W1, W2>::rendezvous()
{
}

template <typename W1, typename W2>
void rendezvous<W1, W2>::add_event(_event_superbase *e, const W1 &w1, const W2 &w2)
{
    uintptr_t u;
    evtrec &ew = _bs.allocate_passive(u);
    (void) new(static_cast<void *>(&ew.w1)) W1(w1);
    (void) new(static_cast<void *>(&ew.w2)) W2(w2);
    _add_event(e, u);
}

template <typename W1, typename W2>
void rendezvous<W1, W2>::_complete(uintptr_t rname, bool success)
{
    if (success) {
	_bs.activate(rname);
	_unblock();
    } else
	_bs.kill(rname);
}

template <typename W1, typename W2>
bool rendezvous<W1, W2>::join(W1 &w1, W2 &w2)
{
    if (evtrec *e = _bs.active_front()) {
	w1 = e->w1;
	w2 = e->w2;
	_bs.pop_active();
	return true;
    } else
	return false;
}


template <typename W1>
class rendezvous<W1, void> : public _rendezvous_base { public:

    rendezvous();

    void add_event(_event_superbase *e, const W1 &w1);
    void _complete(uintptr_t rname, bool success);
    bool join(W1 &);

    unsigned nactive() const	{ return _bs.nactive(); }
    unsigned npassive() const	{ return _bs.npassive(); }
    unsigned nevents() const	{ return nactive() + npassive(); }

  private:

    struct evtrec {
	size_t next;
	W1 w1;
    };

    blockset<evtrec> _bs;
    
};

template <typename W1>
rendezvous<W1, void>::rendezvous()
{
}

template <typename W1>
void rendezvous<W1, void>::add_event(_event_superbase *e, const W1 &w1)
{
    uintptr_t u;
    evtrec &erec = _bs.allocate_passive(u);
    (void) new(static_cast<void *>(&erec.w1)) W1(w1);
    _add_event(e, u);
}

template <typename W1>
void rendezvous<W1, void>::_complete(uintptr_t rname, bool success)
{
    if (success) {
	_bs.activate(rname);
	_unblock();
    } else
	_bs.cancel(rname);
}

template <typename W1>
bool rendezvous<W1, void>::join(W1 &w1)
{
    if (evtrec *e = _bs.active_front()) {
	w1 = e->w1;
	_bs.pop_active();
	return true;
    } else
	return false;
}


template <>
class rendezvous<uintptr_t> : public _rendezvous_base { public:

    inline rendezvous();

    inline void add_event(_event_superbase *e, uintptr_t w1) throw ();
    inline void _complete(uintptr_t rname, bool success);
    inline bool join(uintptr_t &);

    unsigned nactive() const	{ return _buf.size(); }
    unsigned npassive() const	{ return _npassive; }
    unsigned nevents() const	{ return nactive() + npassive(); }

  private:

    debuffer<uintptr_t> _buf;
    size_t _npassive;
    
};

inline rendezvous<uintptr_t>::rendezvous()
    : _npassive(0)
{
}

inline void rendezvous<uintptr_t>::add_event(_event_superbase *e, uintptr_t w1) throw ()
{
    _npassive++;
    _add_event(e, w1);
}

inline void rendezvous<uintptr_t>::_complete(uintptr_t rname, bool success)
{
    if (success) {
	_buf.push_back(rname);
	_unblock();
    }
    _npassive--;
}

inline bool rendezvous<uintptr_t>::join(uintptr_t &w1)
{
    if (uintptr_t *x = _buf.front()) {
	w1 = *x;
	_buf.pop_front();
	return true;
    } else
	return false;
}



template <typename T>
class rendezvous<T *> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add_event(_event_superbase *e, T *w1) throw () {
	inherited::add_event(e, reinterpret_cast<uintptr_t>(w1));
    }
    
    inline bool join(T *&w1) {
	uintptr_t u;
	if (inherited::join(u)) {
	    w1 = reinterpret_cast<T *>(u);
	    return true;
	} else
	    return false;
    }

};


template <>
class rendezvous<int> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add_event(_event_superbase *e, int w1) throw () {
	inherited::add_event(e, static_cast<uintptr_t>(w1));
    }
    
    inline bool join(int &w1) {
	uintptr_t u;
	if (inherited::join(u)) {
	    w1 = static_cast<int>(u);
	    return true;
	} else
	    return false;
    }

};


template <>
class rendezvous<bool> : public rendezvous<uintptr_t> { public:

    typedef rendezvous<uintptr_t> inherited;

    rendezvous() { }

    inline void add_event(_event_superbase *e, bool w1) throw () {
	inherited::add_event(e, static_cast<uintptr_t>(w1));
    }
    
    inline bool join(bool &w1) {
	uintptr_t u;
	if (inherited::join(u)) {
	    w1 = static_cast<bool>(u);
	    return true;
	} else
	    return false;
    }

};


template <>
class rendezvous<void> : public _rendezvous_base { public:

    rendezvous() : _npassive(0), _nactive(0) { }

    inline void add_event(_event_superbase *e) throw () {
	_npassive++;
	_add_event(e, 1);
    }
    
    inline void _complete(uintptr_t rname, bool success) {
	_npassive--;
	if (success) {
	    _nactive++;
	    _unblock();
	}
    }
    
    inline bool join() {
	if (_nactive) {
	    _nactive--;
	    return true;
	} else
	    return false;
    }

    unsigned nactive() const	{ return _nactive; }
    unsigned npassive() const	{ return _npassive; }
    unsigned nevents() const	{ return nactive() + npassive(); }

    static rendezvous<> dead;
    
  protected:

    unsigned _npassive;
    unsigned _nactive;
    
};


class gather_rendezvous : public rendezvous<> { public:

    gather_rendezvous() {
    }

    inline void _complete(uintptr_t rname, bool success) {
	_npassive--;
	if (success)
	    _nactive++;
	if (_npassive == 0)
	    _unblock();
    }
    
};

}
#endif /* TAMER_TAME_RENDEZVOUS_HH */
