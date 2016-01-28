#ifndef TAMER_DINTERNAL_HH
#define TAMER_DINTERNAL_HH 1
#include <tamer/event.hh>
#include <tamer/driver.hh>
#include <sys/types.h>
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
    inline const driver_fd<T>& operator[](int fd) const;
    inline driver_fd<T>& operator[](int fd);

  private:
    enum { fdblksiz = 256 };
    driver_fd<T>** fdblk_;
    driver_fd<T>* fdblk0_;
    unsigned nfds_;
    unsigned fdcap_;
    unsigned changedfd1_;
    driver_fd<T> fdalign_[0];
    char fdspace_[sizeof(driver_fd<T>) * fdblksiz];

    inline driver_fd<T>& at(unsigned fd);
    inline const driver_fd<T>& at(unsigned fd) const;
    template <typename O> void hard_expand(O owner, int need_fd);
};

struct driver_asapset {
    inline driver_asapset();
    ~driver_asapset();

    inline bool empty() const;
    inline void push(simple_event* se);
    inline void pop_trigger();

  private:
    simple_event** ses_;
    unsigned head_;
    unsigned tail_;
    unsigned capmask_;

    void expand();
};

struct driver_timerset {
    inline driver_timerset();
    ~driver_timerset();

    inline bool empty() const;
    inline bool has_foreground() const;
    inline const timeval &expiry() const;
    inline void cull();
    void push(timeval when, simple_event* se, bool bg);
    inline void pop_trigger();

    void check();

  private:
    struct trec {
        timeval when;
        unsigned order;
        simple_event* se;
        inline bool operator<(const trec &x) const;
        inline void clean();
    };

    enum { arity = 4 };
    trec *ts_;
    mutable unsigned nts_;
    mutable unsigned nfg_;
    unsigned rand_;
    unsigned tcap_;
    unsigned order_;

    static inline unsigned heap_parent(unsigned i);
    static inline unsigned heap_first_child(unsigned i);
    inline unsigned heap_last_child(unsigned i) const;
    void hard_cull(unsigned pos) const;
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
inline driver_fd<T>& driver_fdset<T>::at(unsigned fd) {
    return fdblk_[fd / fdblksiz][fd % fdblksiz];
}

template <typename T>
inline const driver_fd<T>& driver_fdset<T>::at(unsigned fd) const {
    return fdblk_[fd / fdblksiz][fd % fdblksiz];
}

template <typename T>
inline driver_fdset<T>::driver_fdset()
    : fdblk_(&fdblk0_), fdblk0_(reinterpret_cast<driver_fd<T>*>(fdspace_)),
      nfds_(0), fdcap_(fdblksiz), changedfd1_(0) {
}

template <typename T>
inline driver_fdset<T>::~driver_fdset() {
    for (unsigned i = 0; i < nfds_; ++i)
        at(i).~driver_fd<T>();
    for (unsigned i = 1; i < fdcap_ / fdblksiz; ++i)
        delete[] reinterpret_cast<char*>(fdblk_[i]);
    if (fdcap_ > fdblksiz)
        delete[] fdblk_;
}

template <typename T> template <typename O>
void driver_fdset<T>::expand(O owner, int need_fd) {
    if (need_fd >= (int) nfds_)
        hard_expand(owner, need_fd);
}

template <typename T> template <typename O>
void driver_fdset<T>::hard_expand(O owner, int need_fd) {
    if (need_fd >= (int) fdcap_) {
        unsigned newfdcap = (need_fd | (fdblksiz - 1)) + 1;
        driver_fd<T>** newfdblk = new driver_fd<T>*[newfdcap / fdblksiz];
        for (unsigned i = 0; i < newfdcap / fdblksiz; ++i)
            if (i < fdcap_ / fdblksiz)
                newfdblk[i] = fdblk_[i];
            else
                newfdblk[i] = reinterpret_cast<driver_fd<T>*>(new char[sizeof(driver_fd<T>) * fdblksiz]);
        if (fdcap_ > fdblksiz)
            delete[] fdblk_;
        fdblk_ = newfdblk;
        fdcap_ = newfdcap;
    }

    while (need_fd >= (int) nfds_) {
        new((void *) &at(nfds_)) driver_fd<T>(owner, nfds_);
        ++nfds_;
    }
}

template <typename T>
inline bool driver_fdset<T>::has_change() const {
    return changedfd1_ != 0;
}

template <typename T>
inline void driver_fdset<T>::push_change(int fd) {
    assert(fd >= 0 && (unsigned) fd < nfds_);
    if (at(fd).next_changedfd1 == 0) {
        at(fd).next_changedfd1 = changedfd1_;
        changedfd1_ = fd + 1;
    }
}

template <typename T>
inline int driver_fdset<T>::pop_change() {
    int fd = changedfd1_ - 1;
    if (fd >= 0) {
        changedfd1_ = at(fd).next_changedfd1;
        at(fd).next_changedfd1 = 0;
    }
    return fd;
}

template <typename T>
inline int driver_fdset<T>::size() const {
    return nfds_;
}

template <typename T>
inline const driver_fd<T>& driver_fdset<T>::operator[](int fd) const {
    assert((unsigned) fd < nfds_);
    return at(fd);
}

template <typename T>
inline driver_fd<T>& driver_fdset<T>::operator[](int fd) {
    assert((unsigned) fd < nfds_);
    return at(fd);
}

inline void* make_fd_callback(const driver* d, int fd) {
    uintptr_t x = d->index() + fd * driver::capacity;
    return reinterpret_cast<void*>(x);
}

inline driver* fd_callback_driver(void* callback) {
    uintptr_t x = reinterpret_cast<uintptr_t>(callback);
    return driver::by_index(x % driver::capacity);
}

inline int fd_callback_fd(void* callback) {
    uintptr_t x = reinterpret_cast<uintptr_t>(callback);
    return x / driver::capacity;
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

inline void driver_asapset::pop_trigger() {
    assert(head_ != tail_);
    simple_event *se = ses_[head_ & capmask_];
    ++head_;
    se->simple_trigger(false);
}

inline driver_timerset::driver_timerset()
    : ts_(0), nts_(0), nfg_(0), rand_(8173), tcap_(0), order_(0) {
}

inline bool driver_timerset::empty() const {
    return nts_ == 0;
}

inline bool driver_timerset::has_foreground() const {
    return nfg_ != 0;
}

inline const timeval &driver_timerset::expiry() const {
    assert(nts_ != 0);
    return ts_[0].when;
}

inline void driver_timerset::cull() {
    while (nts_ != 0 && ts_[0].se->empty())
        hard_cull(0);
}

inline bool driver_timerset::trec::operator<(const trec &x) const {
    return when.tv_sec < x.when.tv_sec
        || (when.tv_sec == x.when.tv_sec
            && (when.tv_usec < x.when.tv_usec
                || (when.tv_usec == x.when.tv_usec
                    && (int) (order - x.order) < 0)));
}

inline void driver_timerset::trec::clean() {
    simple_event::unuse_clean(se);
}

inline unsigned driver_timerset::heap_parent(unsigned i) {
    return (i - (arity == 2)) / arity;
}

inline unsigned driver_timerset::heap_first_child(unsigned i) {
    return i * arity + (arity == 2 || i == 0);
}

inline unsigned driver_timerset::heap_last_child(unsigned i) const {
    unsigned p = i * arity + arity + (arity == 2);
    return p < nts_ ? p : nts_;
}

inline void driver_timerset::pop_trigger() {
    hard_cull((unsigned) -1);
}

} // namespace tamerpriv
} // namespace tamer
#endif
