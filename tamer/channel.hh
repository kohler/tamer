// -*- mode: c++ -*-
#ifndef TAMER_CHANNEL_HH
#define TAMER_CHANNEL_HH 1
#include <deque>
#include <tamer/event.hh>
namespace tamer {

template <typename T>
class channel {
  public:
    typedef typename std::deque<T>::size_type size_type;

    inline channel();

    inline size_type size() const;
    inline bool empty() const;

    inline void push_back(T x);
    inline void pop_front(tamer::event<T> e);

  private:
    std::deque<T> vq_;
    std::deque<tamer::event<T> > eq_;

};

template <typename T>
inline channel<T>::channel() {
}

template <typename T>
inline typename channel<T>::size_type channel<T>::size() const {
    return vq_.size();
}

template <typename T>
inline bool channel<T>::empty() const {
    return vq_.empty();
}

template <typename T>
inline void channel<T>::push_back(T x) {
    while (!eq_.empty() && !eq_.front())
        eq_.pop_front();
    if (!eq_.empty()) {
        eq_.front()(TAMER_MOVE(x));
        eq_.pop_front();
    } else
        vq_.push_back(TAMER_MOVE(x));
}

template <typename T>
inline void channel<T>::pop_front(tamer::event<T> e) {
    if (!vq_.empty()) {
        e(vq_.front());
        vq_.pop_front();
    } else
        eq_.push_back(TAMER_MOVE(e));
}

}
#endif
