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

    inline size_type wait_size() const;
    inline bool wait_empty() const;

    inline void push_back(T x);
    inline void pop_front(tamer::event<T> e);

  private:
    std::deque<T> q_;
    std::deque<tamer::event<T> > waitq_;

};

template <typename T>
inline channel<T>::channel() {
}

template <typename T>
inline typename channel<T>::size_type channel<T>::size() const {
    return q_.size();
}

template <typename T>
inline bool channel<T>::empty() const {
    return q_.empty();
}

template <typename T>
inline typename channel<T>::size_type channel<T>::wait_size() const {
    return waitq_.size();
}

template <typename T>
inline bool channel<T>::wait_empty() const {
    return waitq_.empty();
}

template <typename T>
inline void channel<T>::push_back(T x) {
    while (!waitq_.empty() && !waitq_.front())
        waitq_.pop_front();
    if (!waitq_.empty()) {
        waitq_.front()(TAMER_MOVE(x));
        waitq_.pop_front();
    } else
        q_.push_back(TAMER_MOVE(x));
}

template <typename T>
inline void channel<T>::pop_front(tamer::event<T> e) {
    if (!q_.empty()) {
        e(q_.front());
        q_.pop_front();
    } else
        waitq_.push_back(TAMER_MOVE(e));
}

}
#endif
