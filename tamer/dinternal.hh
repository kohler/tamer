#ifndef TAMER_DINTERNAL_HH
#define TAMER_DINTERNAL_HH 1
#include <tamer/event.hh>
#include <string.h>
namespace tamer {
namespace tamerpriv {

template <typename T>
struct driver_fd : public T {
    event<int> e[2];
    int next_changedfd1;

    template <typename O> inline driver_fd(O owner, int fd);
    inline bool empty() const;
};

template <typename T>
struct driver_fdset {
    inline driver_fdset();
    inline ~driver_fdset();

    template <typename O> inline void expand(O owner, int need_fd);
    inline void push_change(int fd);
    inline int pop_change();
    inline bool has_change() const;

    inline int size() const;
    inline const driver_fd<T> &operator[](int fd) const;
    inline driver_fd<T> &operator[](int fd);

  private:
    driver_fd<T> *fds_;
    int nfds_;
    int fdcap_;
    int changedfd1_;

    template <typename O> void hard_expand(O owner, int need_fd);
};

struct driver_asapset {
    inline driver_asapset();
    ~driver_asapset();

    inline bool empty() const;
    inline void push(simple_event *se);
    inline simple_event *pop();

  private:
    simple_event **ses_;
    unsigned head_;
    unsigned tail_;
    unsigned capmask_;

    void expand();
};


template <typename T> template <typename O>
inline driver_fd<T>::driver_fd(O owner, int fd)
    : T(owner, fd), next_changedfd1(0) {
}

template <typename T>
inline bool driver_fd<T>::empty() const {
    return e[0].empty() && e[1].empty();
}

template <typename T>
inline driver_fdset<T>::driver_fdset()
    : fds_(), nfds_(0), fdcap_(0), changedfd1_(0) {
}

template <typename T>
inline driver_fdset<T>::~driver_fdset() {
    for (int i = 0; i < nfds_; ++i)
	fds_[i].~driver_fd<T>();
    delete[] reinterpret_cast<char *>(fds_);
}

template <typename T> template <typename O>
void driver_fdset<T>::expand(O owner, int need_fd) {
    if (need_fd >= nfds_)
	hard_expand(owner, need_fd);
}

template <typename T> template <typename O>
void driver_fdset<T>::hard_expand(O owner, int need_fd) {
    if (need_fd >= fdcap_) {
	int newcap = (fdcap_ ? (fdcap_ + 2) * 2 - 2 : 30);
	while (newcap <= need_fd)
	    newcap = (newcap + 2) * 2 - 2;

	driver_fd<T> *newfds =
	    reinterpret_cast<driver_fd<T> *>(new char[sizeof(driver_fd<T>) * newcap]);
	memcpy(newfds, fds_, sizeof(driver_fd<T>) * nfds_);
	for (int i = 0; i < nfds_; ++i)
	    newfds[i].move(owner, i, fds_[i]);
	// XXX Relies on being able to memcpy() event<int>.

	delete[] reinterpret_cast<char *>(fds_);
	fds_ = newfds;
	fdcap_ = newcap;
    }

    while (need_fd >= nfds_) {
	new((void *) &fds_[nfds_]) driver_fd<T>(owner, nfds_);
	++nfds_;
    }
}

template <typename T>
inline bool driver_fdset<T>::has_change() const {
    return changedfd1_ != 0;
}

template <typename T>
inline void driver_fdset<T>::push_change(int fd) {
    assert(fd >= 0 && fd < nfds_);
    if (fds_[fd].next_changedfd1 == 0) {
	fds_[fd].next_changedfd1 = changedfd1_;
	changedfd1_ = fd + 1;
    }
}

template <typename T>
inline int driver_fdset<T>::pop_change() {
    int fd = changedfd1_ - 1;
    if (fd >= 0) {
	changedfd1_ = fds_[fd].next_changedfd1;
	fds_[fd].next_changedfd1 = 0;
    }
    return fd;
}

template <typename T>
inline int driver_fdset<T>::size() const {
    return nfds_;
}

template <typename T>
inline const driver_fd<T> &driver_fdset<T>::operator[](int fd) const {
    assert((unsigned) fd < (unsigned) nfds_);
    return fds_[fd];
}

template <typename T>
inline driver_fd<T> &driver_fdset<T>::operator[](int fd) {
    assert((unsigned) fd < (unsigned) nfds_);
    return fds_[fd];
}

inline driver_asapset::driver_asapset()
    : ses_(), head_(0), tail_(0), capmask_(~0U) {
}

inline bool driver_asapset::empty() const {
    return head_ == tail_;
}

inline void driver_asapset::push(simple_event *se) {
    if (tail_ - head_ == capmask_ + 1)
	expand();
    ses_[tail_ & capmask_] = se;
    ++tail_;
}

inline simple_event *driver_asapset::pop() {
    assert(head_ != tail_);
    simple_event *se = ses_[head_ & capmask_];
    ++head_;
    return se;
}

} // namespace tamerpriv
} // namespace tamer
#endif
