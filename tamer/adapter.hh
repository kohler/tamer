#ifndef TAMER_TAME_ADAPTER_HH
#define TAMER_TAME_ADAPTER_HH 1
#include "tame_event.hh"
namespace tame {

template <typename T> class _unbind_rendezvous : public _rendezvous_superbase { public:

    _unbind_rendezvous(const event<> &ein)
	: _ein(ein), _eout(this, ubtrigger)
    {
	_ein.setcancel(event<>(this, ubcancel));
    }

    void _complete(uintptr_t rname, bool success) {
	if (success) {
	    if (rname == ubcancel)
		_eout.cancel();
	    else
		_ein.trigger();
	    delete this;
	}
    }

    const event<T> &eout() const {
	return _eout;
    }

  private:

    event<> _ein;
    event<T> _eout;
    
    enum { ubcancel = 0, ubtrigger = 1 };
    
};

template <typename T1> class _bind_rendezvous : public _rendezvous_superbase { public:

    template <typename X1>
    _bind_rendezvous(const event<T1> &ein, const X1 &t1)
	: _ein(ein), _eout(this, ubtrigger), _t1(t1)
    {
	_ein.setcancel(event<>(this, ubcancel));
    }

    ~_bind_rendezvous() {
    }

    void _complete(uintptr_t rname, bool success) {
	if (success) {
	    if (rname == ubcancel)
		_eout.cancel();
	    else
		_ein.trigger(_t1);
	    delete this;
	}
    }

    const event<> &eout() const {
	return _eout;
    }
    
  private:

    event<T1> _ein;
    event<> _eout;
    T1 _t1;

    enum { ubcancel = 0, ubtrigger = 1 };
    
};

template <typename T1>
const event<T1> &unbind(const event<> &e) {
    _unbind_rendezvous<T1> *ur = new _unbind_rendezvous<T1>(e);
    return ur->eout();
}

template <typename T1, typename X1>
const event<> &bind(const event<T1> &e, const X1 &t1) {
    _bind_rendezvous<T1> *ur = new _bind_rendezvous<T1>(e, t1);
    return ur->eout();
}

}
#endif /* TAMER_TAME_ADAPTER_HH */
