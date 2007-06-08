#ifndef TAMER_XEVENT_HH
#define TAMER_XEVENT_HH 1
#include <tamer/rendezvous.hh>
namespace tamer {
namespace tamerpriv {

template <typename R, typename I0, typename I1>
inline simple_event::simple_event(R &r, const I0 &i0, const I1 &i1)
    : _refcount(1), _at_trigger(0)
{
    r.add(this, i0, i1);
}

template <typename R, typename I0>
inline simple_event::simple_event(R &r, const I0 &i0)
    : _refcount(1), _at_trigger(0)
{
    r.add(this, i0);
}

template <typename R>
inline simple_event::simple_event(R &r)
    : _refcount(1), _at_trigger(0)
{
    r.add(this);
}

}}
#endif /* TAMER_XEVENT_HH */
