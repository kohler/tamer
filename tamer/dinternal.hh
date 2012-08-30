#ifndef TAMER_DINTERNAL_HH
#define TAMER_DINTERNAL_HH 1
#include <string.h>
namespace tamer {

template <typename T>
struct driver_fd : public T {
    event<int> e[2];
    int next_changedfd1;

    template <typename O>
    inline driver_fd(O owner, int fd)
	: T(owner, fd), next_changedfd1(0) {
    }
    inline bool empty() const {
	return e[0].empty() && e[1].empty();
    }
};

template <typename T>
struct driver_fdset {
    inline driver_fdset();
    inline ~driver_fdset();

    template <typename O> inline void expand(O owner, int need_fd);
    inline void push_change(int fd);
    inline int pop_change();
    inline bool has_change() const;

    inline const driver_fd<T> &operator[](int fd) const;
    inline driver_fd<T> &operator[](int fd);

    driver_fd<T> *fds_;
    int nfds_;
    int fdcap_;
    int changedfd1_;

  private:
    template <typename O> void hard_expand(O owner, int need_fd);
};

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
inline const driver_fd<T> &driver_fdset<T>::operator[](int fd) const {
    assert((unsigned) fd < (unsigned) nfds_);
    return fds_[fd];
}

template <typename T>
inline driver_fd<T> &driver_fdset<T>::operator[](int fd) {
    assert((unsigned) fd < (unsigned) nfds_);
    return fds_[fd];
}

} // namespace tamer
#endif
